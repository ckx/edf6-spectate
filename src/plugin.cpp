#include <windows.h>

#pragma warning(push)
#pragma warning(disable : 4201)
#include "PluginAPI.h"
#pragma warning(pop)

#include "spectate.h"
#include "log.h"

namespace {

#define SPECTATE_TEXT2(x) #x
#define SPECTATE_TEXT(x) SPECTATE_TEXT2(x)
constexpr const char* kVersion = SPECTATE_TEXT(SPECTATE_VERSION_MAJOR) "." SPECTATE_TEXT(
    SPECTATE_VERSION_MINOR) "." SPECTATE_TEXT(SPECTATE_VERSION_PATCH);

HMODULE self = nullptr;

// <plugin folder>\EDF6Spectate.log
bool LogPath(wchar_t (&path)[MAX_PATH]) {
    const DWORD length = GetModuleFileNameW(self, path, MAX_PATH);
    if (!length || length >= MAX_PATH - 4) return false;
    wchar_t* dot = nullptr;
    for (wchar_t* at = path; *at; ++at)
        if (*at == L'.') dot = at;
        else if (*at == L'\\' || *at == L'/') dot = nullptr;
    if (!dot) return false;
    lstrcpyW(dot, L".log");
    return true;
}

}  // namespace

// EDFModLoader calls this once at startup. False unloads the plugin at once; true keeps it for the life of the
// process, since the game's System::Update call then leads into it.
extern "C" __declspec(dllexport) bool EDFMLAPI EML6_Load(PluginInfo* info) {
    if (!info) return false;
    info->infoVersion = PluginInfo::MaxInfoVer;
    info->name = "EDF6Spectate";
    info->version = PLUG_VER(SPECTATE_VERSION_MAJOR, SPECTATE_VERSION_MINOR, SPECTATE_VERSION_PATCH, 0);
    wchar_t logPath[MAX_PATH]{};
    if (LogPath(logPath)) spectate::LogOpen(logPath);
    spectate::Log("==== EDF6Spectate %s ====", kVersion);
    const HMODULE game = GetModuleHandleW(L"EDF.dll");
    if (!game) {
        spectate::Log("REFUSED: EDF.dll is not loaded, so this is not the game; nothing was changed");
        return false;
    }
    return spectate::Install(reinterpret_cast<unsigned char*>(game));
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        self = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
