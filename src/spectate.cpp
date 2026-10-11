#include "spectate.h"

#include <windows.h>

#include <cstring>

#include "guard.h"
#include "log.h"

namespace spectate {
namespace {

constexpr std::uint32_t kGhostCameraFactory = 0x82B40;  // System's factory for CharacterGhostCamera
constexpr std::uint32_t kWeakFromThis = 0x1198950;      // (SceneCamera*, weak_ptr* out)
constexpr std::uint32_t kSetViewCamera = 0x1193FA0;     // (View*, weak_ptr<SceneCamera> by value)
constexpr std::uint32_t kIsOnlineMode = 0x1B5DF0;       // the mission script's bool IsOnlineMode()
constexpr std::uint32_t kDynamicCast = 0x12DA7AA;       // __RTDynamicCast
constexpr std::uint32_t kGameObjectBaseType = 0x2006400;  // RTTI type descriptors
constexpr std::uint32_t kSoldierBaseType = 0x2006428;
constexpr std::uint32_t kTeamManager = 0x20B2978;  // TeamManager*
constexpr std::uint32_t kGhostCameraVtable = 0x1768C10;
constexpr std::uint32_t kGhostCameraParamVtable = 0x1762168;

// System: four views of 0x188 bytes from +0x4D0. A view holds its current camera as a weak_ptr at +0x8 and its
// cameras in a std::list<shared_ptr<SceneCamera>> at +0x18 (nodes: next +0, value +0x10). Online, the local
// player's camera is on view 0.
constexpr std::size_t kView0 = 0x4D0;
constexpr std::size_t kViewCurrent = 0x8;
constexpr std::size_t kViewCameras = 0x18;
constexpr std::size_t kNodeValue = 0x10;
// CharacterGhostCamera follows the object in the weak_ptr at +0x350, taken from its InitParam's +0x80.
constexpr std::size_t kCameraTarget = 0x350;
constexpr std::size_t kParamTarget = 0x80;
constexpr std::size_t kParamSize = 0xA0;  // the game's own are 0x88 bytes
// TeamManager +0x38 points at the players' std::map. Tree nodes: left +0, parent +8, right +0x10, nil flag +0x19,
// the player at +0x20.
constexpr std::size_t kTeams = 0x38;
constexpr std::size_t kTreeLeft = 0x0;
constexpr std::size_t kTreeParent = 0x8;
constexpr std::size_t kTreeRight = 0x10;
constexpr std::size_t kTreeNil = 0x19;
constexpr std::size_t kTreeObject = 0x20;
// SoldierBase
constexpr std::size_t kRemote = 0x128;        // bit 0: a remote player's copy
constexpr std::size_t kHealth = 0x2F8;        // float; the script's IsAlive is health > 0, and a downed player has 0
constexpr std::size_t kLocalControl = 0x340;  // set on a locally controlled player
constexpr std::size_t kNetPlayer = 0x1ED0;    // a network player's record, with its slot at +0x48
constexpr std::size_t kNetSlot = 0x48;
// shared_ptr control block (MSVC): vtable (+0 destroy the object, +8 delete the block), uses +8, weaks +0xC.
constexpr std::size_t kUses = 0x8;
constexpr std::size_t kWeaks = 0xC;
// Well above the four players and the handful of cameras a view holds: anything longer is not the game's.
constexpr int kMaxNodes = 64;
constexpr std::size_t kMaxPlayers = 8;

using SystemUpdateFn = void (*)(void* system, float step, std::uint32_t frames);
using FactoryFn = void* (*)(void* system, void* out, std::uint32_t view, const float* matrix, void* param);
using WeakFromThisFn = void* (*)(void* camera, void* out);
using SetViewCameraFn = void (*)(void* view, void* weak);
using IsOnlineModeFn = bool (*)();
using DynamicCastFn = void* (*)(void* object, long vfDelta, void* sourceType, void* targetType, int isReference);
using ControlFn = void (*)(std::uintptr_t control);

// Game code the plugin calls or relies on, by FNV-1a 64 of its bytes. None of these ranges holds a relocation.
struct Code {
    const char* name;
    std::uint32_t rva;
    std::uint32_t size;
    std::uint64_t hash;
};
constexpr Code kCode[] = {
    {"System::Update call", kSystemUpdateCall, 0x5, 0xF9744FB4304DB3F6},
    {"System::Update", kSystemUpdate, 0x9FB, 0x7F0564B7A3BF2724},
    {"CharacterGhostCamera factory", kGhostCameraFactory, 0x1DC, 0x6F6DB55CC4005B3C},
    {"view camera switch", kSetViewCamera, 0x18D, 0xF859E0B97E448F2F},
    {"weak_from_this", kWeakFromThis, 0x9C, 0x580BFED010D5E37E},
    {"camera flush", 0x1195A80, 0x159, 0xFAAA5CBBF9469555},
    {"current camera update", 0x11978D0, 0x1000, 0x15240CC9F660A4DA},
    {"create_remote_camera", 0xCCCA0, 0x2D9, 0x7B438E325C57D000},
    {"CharacterGhostCamera constructor", 0xF4860, 0x974, 0xB9D5E9BDAA015326},
    {"IsPlayer", 0x550890, 0xA, 0x1D923A0DEC9EF9A4},
    {"IsAlive", 0x1C0BE0, 0x2E, 0xA905EB899CAEB9D9},
    {"IsOnlineMode", kIsOnlineMode, 0x2B, 0xA71A18502F48BC92},
    {"__RTDynamicCast import thunk", kDynamicCast, 0x6, 0xD6E2BD7734AF1D4E},
};

std::uint64_t Fnv1a(const unsigned char* bytes, std::size_t size) {
    std::uint64_t hash = 0xCBF29CE484222325;
    for (std::size_t i = 0; i < size; ++i) hash = (hash ^ bytes[i]) * 0x100000001B3;
    return hash;
}

// Reads the headers of the module the loader found as EDF.dll, which are mapped and readable.
bool SupportedImage(const unsigned char* image) {
    IMAGE_DOS_HEADER dos{};
    std::memcpy(&dos, image, sizeof(dos));
    if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 0x1000) return false;
    IMAGE_NT_HEADERS64 nt{};
    std::memcpy(&nt, image + dos.e_lfanew, sizeof(nt));
    return nt.Signature == IMAGE_NT_SIGNATURE && nt.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 &&
           nt.FileHeader.TimeDateStamp == kImageTimeDateStamp && nt.OptionalHeader.SizeOfImage == kImageSize;
}

template <typename T>
T At(std::uintptr_t address) {
    T value;
    std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(T));
    return value;
}

std::uintptr_t Ptr(std::uintptr_t address) { return At<std::uintptr_t>(address); }

bool Expired(std::uintptr_t control) { return !control || At<std::int32_t>(control + kUses) <= 0; }

unsigned char* game = nullptr;
SystemUpdateFn systemUpdate = nullptr;
bool stopped = false;
// A slot no player has: the current camera follows someone who left the room.
constexpr std::uint32_t kGone = kOwnCamera - 1;
std::uint32_t createdThisDown = 0;  // slots whose camera was asked for since the local player went down

struct Player {
    std::uintptr_t object;
    std::uint32_t slot;
    bool alive;
};

struct Roster {
    Player local{};
    bool hasLocal = false;
    Player remote[kMaxPlayers]{};  // in slot order
    std::size_t remotes = 0;
};

// The players in TeamManager's map, as the game's create_remote_camera (CCCA0) walks it.
Roster ReadRoster() {
    Roster roster;
    const auto cast = reinterpret_cast<DynamicCastFn>(game + kDynamicCast);
    const std::uintptr_t teams = Ptr(Ptr(reinterpret_cast<std::uintptr_t>(game) + kTeamManager) + kTeams);
    const auto nil = [](std::uintptr_t node) { return At<std::uint8_t>(node + kTreeNil) != 0; };
    std::uintptr_t node = Ptr(Ptr(teams) + kTreeLeft);  // the head's left is the first node
    for (int visited = 0; !nil(node) && visited < kMaxNodes; ++visited) {
        void* object = cast(reinterpret_cast<void*>(Ptr(node + kTreeObject)), 0, game + kGameObjectBaseType,
                            game + kSoldierBaseType, 0);
        if (object) {
            const auto soldier = reinterpret_cast<std::uintptr_t>(object);
            const std::uintptr_t net = Ptr(soldier + kNetPlayer);
            const Player player{soldier, net ? At<std::uint32_t>(net + kNetSlot) : kOwnCamera,
                                At<float>(soldier + kHealth) > 0.0f};
            const bool remote = (At<std::uint32_t>(soldier + kRemote) & 1) != 0;
            if (!remote && Ptr(soldier + kLocalControl) && !roster.hasLocal) {
                roster.local = player;
                roster.hasLocal = true;
            } else if (remote && net && roster.remotes < kMaxPlayers) {
                std::size_t at = roster.remotes++;
                for (; at > 0 && roster.remote[at - 1].slot > player.slot; --at)
                    roster.remote[at] = roster.remote[at - 1];
                roster.remote[at] = player;
            }
        }
        // In-order successor.
        if (!nil(Ptr(node + kTreeRight))) {
            node = Ptr(node + kTreeRight);
            for (int i = 0; !nil(Ptr(node + kTreeLeft)) && i < kMaxNodes; ++i) node = Ptr(node + kTreeLeft);
        } else {
            std::uintptr_t parent = Ptr(node + kTreeParent);
            for (int i = 0; !nil(parent) && node == Ptr(parent + kTreeRight) && i < kMaxNodes; ++i) {
                node = parent;
                parent = Ptr(node + kTreeParent);
            }
            node = parent;
        }
    }
    return roster;
}

// The object a ghost camera follows, or 0 when the camera is another kind or its target no longer lives.
std::uintptr_t Followed(std::uintptr_t camera) {
    if (!camera || Ptr(camera) != reinterpret_cast<std::uintptr_t>(game + kGhostCameraVtable)) return 0;
    return Expired(Ptr(camera + kCameraTarget + 8)) ? 0 : Ptr(camera + kCameraTarget);
}

// View 0's ghost camera that follows `object`, or 0.
std::uintptr_t FindCamera(std::uintptr_t view, std::uintptr_t object) {
    const std::uintptr_t sentinel = Ptr(view + kViewCameras);
    std::uintptr_t node = Ptr(sentinel);
    for (int i = 0; node != sentinel && i < kMaxNodes; ++i, node = Ptr(node)) {
        const std::uintptr_t camera = Ptr(node + kNodeValue);
        if (Followed(camera) == object) return camera;
    }
    return 0;
}

// Drops one strong reference, as the game's inlined shared_ptr destructor does.
void Release(std::uintptr_t control) {
    if (!control || InterlockedDecrement(reinterpret_cast<volatile LONG*>(control + kUses)) != 0) return;
    const std::uintptr_t vtable = Ptr(control);
    reinterpret_cast<ControlFn>(Ptr(vtable))(control);
    if (InterlockedDecrement(reinterpret_cast<volatile LONG*>(control + kWeaks)) == 0)
        reinterpret_cast<ControlFn>(Ptr(vtable + 8))(control);
}

// A ghost camera following `object` on view 0, made as the game's create_remote_camera (CCCA0) makes one. The System
// adds it to the view and makes it current in its next flush, which the System::Update about to run does.
void Create(std::uintptr_t system, std::uintptr_t object) {
    alignas(16) const float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    alignas(16) unsigned char param[kParamSize]{};
    const auto vtable = reinterpret_cast<std::uintptr_t>(game + kGhostCameraParamVtable);
    std::memcpy(param, &vtable, sizeof(vtable));
    std::memcpy(param + kParamTarget, &object, sizeof(object));
    alignas(16) std::uintptr_t camera[2]{};  // shared_ptr<CharacterGhostCamera>
    reinterpret_cast<FactoryFn>(game + kGhostCameraFactory)(reinterpret_cast<void*>(system), camera, 0, identity,
                                                             param);
    // The System holds its own reference.
    Release(camera[1]);
}

void Follow(std::uintptr_t system) {
    if (!reinterpret_cast<IsOnlineModeFn>(game + kIsOnlineMode)()) {
        createdThisDown = 0;
        return;
    }
    const std::uintptr_t view = system + kView0;
    const std::uintptr_t current = Ptr(view + kViewCurrent);
    if (!current || Expired(Ptr(view + kViewCurrent + 8))) return;
    // Another kind of camera is none of the plugin's business. None was seen in online play.
    if (Ptr(current) != reinterpret_cast<std::uintptr_t>(game + kGhostCameraVtable)) return;
    const std::uintptr_t followed = Followed(current);

    const Roster roster = ReadRoster();
    if (!roster.hasLocal) return;
    std::uint32_t followedSlot = kOwnCamera;
    if (followed != roster.local.object) {
        // A teammate, or someone who left the room. The ghost camera's update (slot 3, F8210, which 11978D0 runs for
        // each view's current camera only) reads its cached SoldierBase* (+0x360) without checking that the target
        // still lives, so a camera on someone gone must not stay current.
        followedSlot = kGone;
        for (std::size_t i = 0; i < roster.remotes; ++i)
            if (followed && roster.remote[i].object == followed) followedSlot = roster.remote[i].slot;
    }

    std::uint32_t target = kOwnCamera;
    if (roster.local.alive) {
        createdThisDown = 0;
    } else {
        Teammate mates[kMaxPlayers];
        for (std::size_t i = 0; i < roster.remotes; ++i) mates[i] = {roster.remote[i].slot, roster.remote[i].alive};
        target = NextTarget(mates, roster.remotes, followedSlot);
    }
    if (target == followedSlot) return;

    std::uintptr_t object = roster.local.object;
    for (std::size_t i = 0; i < roster.remotes; ++i)
        if (roster.remote[i].slot == target) object = roster.remote[i].object;
    if (const std::uintptr_t camera = FindCamera(view, object)) {
        ShowCamera(game, view, camera);
        if (target == kOwnCamera) Log("Back to the player's own camera");
        else Log("Following the player in slot %u", target);
    } else if (target != kOwnCamera && target < 32 && !(createdThisDown & (1u << target))) {
        createdThisDown |= 1u << target;
        Create(system, object);
        Log("Following the player in slot %u (new camera)", target);
    }
}

void SystemUpdate(void* system, float step, std::uint32_t frames) {
    if (!stopped && system && !Guarded([&] { Follow(reinterpret_cast<std::uintptr_t>(system)); })) {
        stopped = true;
        Log("STOPPED: a fault while following a player; the camera stays as it is until the game restarts");
    }
    systemUpdate(system, step, frames);
}

// Executable memory within rel32 reach of `site`.
unsigned char* AllocateNear(const unsigned char* site, std::size_t size) {
    constexpr std::uintptr_t kReach = 0x7FF00000;
    constexpr std::uintptr_t kGranularity = 0x10000;
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(site) & ~(kGranularity - 1);
    const auto at = [size](std::uintptr_t address) {
        return static_cast<unsigned char*>(
            VirtualAlloc(reinterpret_cast<void*>(address), size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    };
    for (std::uintptr_t step = kGranularity; step < kReach; step += kGranularity) {
        if (unsigned char* page = at(base + step)) return page;
        if (base > step)
            if (unsigned char* page = at(base - step)) return page;
    }
    return nullptr;
}

}  // namespace

std::uint32_t NextTarget(const Teammate* mates, std::size_t count, std::uint32_t followed) {
    std::size_t start = count;
    for (std::size_t i = 0; i < count; ++i) {
        if (mates[i].slot == followed && mates[i].alive) return followed;
        if (start == count && mates[i].slot > followed) start = i;
    }
    for (std::size_t i = 0; i < count; ++i) {
        const Teammate& mate = mates[(start + i) % count];
        if (mate.alive) return mate.slot;
    }
    return kOwnCamera;
}

// As the game's own 11932F0 does after adding a camera to a view.
void ShowCamera(unsigned char* base, std::uintptr_t view, std::uintptr_t camera) {
    alignas(16) std::uintptr_t weak[2]{};
    reinterpret_cast<WeakFromThisFn>(base + kWeakFromThis)(reinterpret_cast<void*>(camera), weak);
    // Takes the weak_ptr by value, so it releases `weak` itself.
    reinterpret_cast<SetViewCameraFn>(base + kSetViewCamera)(reinterpret_cast<void*>(view), weak);
}

bool Install(unsigned char* base) {
    if (!SupportedImage(base)) {
        Log("REFUSED: EDF.dll is not the supported build (TimeDateStamp %08X, SizeOfImage %X); nothing was changed",
            kImageTimeDateStamp, kImageSize);
        return false;
    }
    for (const Code& code : kCode) {
        if (Fnv1a(base + code.rva, code.size) != code.hash) {
            Log("REFUSED: EDF+%X (%s) is not the expected code; nothing was changed", code.rva, code.name);
            return false;
        }
    }
    unsigned char* const site = base + kSystemUpdateCall;
    unsigned char* const stub = AllocateNear(site, 0x1000);
    if (!stub) {
        Log("REFUSED: no memory near EDF.dll for the call stub; nothing was changed");
        return false;
    }
    // mov rax, SystemUpdate; jmp rax. The return address stays the game's, so nothing unwinds through the stub.
    unsigned char code[12] = {0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
    const auto hook = reinterpret_cast<std::uintptr_t>(&SystemUpdate);
    std::memcpy(code + 2, &hook, sizeof(hook));
    std::memcpy(stub, code, sizeof(code));
    DWORD protection = 0;
    if (!VirtualProtect(stub, sizeof(code), PAGE_EXECUTE_READ, &protection)) {
        VirtualFree(stub, 0, MEM_RELEASE);
        Log("REFUSED: could not make the call stub executable (error %lu); nothing was changed", GetLastError());
        return false;
    }
    FlushInstructionCache(GetCurrentProcess(), stub, sizeof(code));
    const auto relative = static_cast<std::int32_t>(stub - (site + 5));
    if (!VirtualProtect(site + 1, sizeof(relative), PAGE_EXECUTE_READWRITE, &protection)) {
        VirtualFree(stub, 0, MEM_RELEASE);
        Log("REFUSED: could not unprotect EDF+%X (error %lu); nothing was changed", kSystemUpdateCall,
            GetLastError());
        return false;
    }
    game = base;
    systemUpdate = reinterpret_cast<SystemUpdateFn>(base + kSystemUpdate);
    std::memcpy(site + 1, &relative, sizeof(relative));
    VirtualProtect(site + 1, sizeof(relative), protection, &protection);
    FlushInstructionCache(GetCurrentProcess(), site, 5);
    Log("Installed: a downed player online watches a living teammate until revived (System::Update call EDF+%X)",
        kSystemUpdateCall);
    return true;
}

}  // namespace spectate
