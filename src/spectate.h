#pragma once
#include <cstddef>
#include <cstdint>

namespace spectate {

// The EDF.dll build every address below was taken from (Steam, 2025-01 update). docs/research.md records the
// evidence for each one.
constexpr std::uint32_t kImageTimeDateStamp = 0x678CCB46;
constexpr std::uint32_t kImageSize = 0x22CE000;

// Application::Update calls System::Update here once a frame, on the main thread, before the frame's cameras are
// drawn. The plugin takes this call.
constexpr std::uint32_t kSystemUpdateCall = 0x705715;
constexpr std::uint32_t kSystemUpdate = 0x1198DA0;

// A remote player in an online room, by the slot the game numbers players with (0 to 3).
struct Teammate {
    std::uint32_t slot;
    bool alive;
};

// No teammate to follow: view 0 shows the local player's own camera.
constexpr std::uint32_t kOwnCamera = 0xFFFFFFFF;

// Who a downed local player watches. `mates` are in slot order; `followed` is the slot followed now, or kOwnCamera.
// Keeps `followed` while that teammate lives, else takes the next living teammate after it in slot order (wrapping),
// else kOwnCamera.
std::uint32_t NextTarget(const Teammate* mates, std::size_t count, std::uint32_t followed);

// Makes `camera`, a SceneCamera some shared_ptr keeps alive, the current camera of `view` through the game's own
// weak_from_this (1198950) and view switch (1193FA0). The view keeps a weak_ptr to it.
void ShowCamera(unsigned char* game, std::uintptr_t view, std::uintptr_t camera);

// The supported EDF.dll and every byte of game code the plugin calls or relies on: then the System::Update call is
// pointed at the plugin, which switches view 0's camera before calling the game's System::Update. False, logged and
// with nothing changed otherwise.
bool Install(unsigned char* game);

}  // namespace spectate
