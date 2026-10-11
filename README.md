# EDF6 Spectate

An EDFModLoader plugin for EARTH DEFENSE FORCE 6. In an online room, a downed player
watches a standing teammate through the game's follow camera until revived.

Status: prototype. Built and tested against the game's code under Wine; not yet run
in a live room.

## Behavior

- Going down online switches your view to the standing teammate with the lowest
  slot. If they go down or leave, it moves to the next one.
- Being revived, or every teammate being down, returns your own camera.
- Offline play and split screen are left alone.
- Nothing is sent to other players. They do not need the plugin.
- Only the supported EDF.dll (Steam, 2025-01 update) is patched. Any other build, or
  code another mod changed, makes the plugin refuse and change nothing.

`Mods/Plugins/EDF6Spectate.log` records whether the plugin installed and each camera
change.

## Documents

- `docs/design.md`: behavior, runtime design and open decisions.
- `docs/research.md`: the game interfaces used and their evidence.
- `docs/implementation-plan.md`: milestones and the live checklist.
- `AGENTS.md`: working rules for contributors and coding agents.

## Build

Linux, cross-compiling with clang for MinGW-w64 and testing under Wine:

```sh
EDF6_GAME_DIR="/path/to/EARTH DEFENSE FORCE 6" ./build.sh -t
```

The plugin lands in `out/EDF6Spectate.dll`. Fedora needs `clang lld mingw64-gcc-c++
mingw64-winpthreads-static wine cmake ninja-build`. `EDF6_GAME_DIR` lets the tests
run the game's own code from `EDF.dll`, which is only read. Without it those tests
report skipped, and `-r` makes the configure fail instead.

Windows requires Visual Studio 2022 with Desktop development with C++ and CMake:

```powershell
./build.ps1 -Test
```

## Installation

Close the game and back up your saves. Copy `EDF6Spectate.dll` into the game's
`Mods/Plugins` folder, next to any other plugins. EDFModLoader must already be
installed (Proton needs the `WINEDLLOVERRIDES="winmm=n,b" %command%` launch option).
Remove the DLL to uninstall.

## Licenses

`src/guard.h` comes from edf-coop-stable's `game/` folder, released into the public
domain (`third_party/EDF6Coop/LICENSE.txt`). `third_party/EDFModLoader/PluginAPI.h`
is MIT licensed (`third_party/EDFModLoader/LICENSE.txt`). This repository has no
chosen license yet.
