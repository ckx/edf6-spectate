# EDF6 Spectate

An EDFModLoader plugin for EARTH DEFENSE FORCE 6. In an online room, a downed player
watches a standing teammate through the game's follow camera until revived.

Status: experimental. Camera following worked in the user's online retry of
0.1.1. Version 0.2.0 adds the spectated teammate's native combat HUD and still
needs an online HUD check.

## Install

Download `EDF6Spectate-0.2.0.zip` from the experimental release:

https://github.com/ckx/edf6-spectate/releases/tag/v0.2.0

Close EDF6, back up your saves, and extract the whole ZIP to a separate folder.
Use the release ZIP, not GitHub's source-code archives.

- **Windows:** double-click `INSTALL.bat`. It finds EDF6 in your Steam libraries
  or asks for the game folder.
- **Linux / Proton:** run `bash install.sh` in the extracted folder. If needed,
  pass the game folder as its argument. Add `WINEDLLOVERRIDES="winmm=n,b"` to the
  Steam launch command. For an empty launch-options field, use
  `WINEDLLOVERRIDES="winmm=n,b" %command%`. Merge existing DLL overrides and keep
  your other launch options.

The ZIP includes EDFModLoader for fresh installations. Installers preserve an
existing `winmm.dll`, other mods, settings and launch options. If `ModLoader.ini`
exists, it must permit loading plugins. EDF6Coop is not required.

Start from Steam as usual. Check `Mods/Plugins/EDF6Spectate.log` for `Installed:`.
For the first online test, go down, watch a teammate move, and get revived. Camera
handover, disconnects and mission transitions still need live verification.

Upgrade by running the newer installer with the game closed. Uninstall with
`UNINSTALL.bat` or `bash install.sh --uninstall`. This removes only the spectator
DLL; the loader, other mods and logs remain.

## Behavior

- Going down online switches your view to the standing teammate with the lowest
  slot. If they go down or leave, it moves to the next one.
- Being revived, or every teammate being down, returns your own camera.
- The followed teammate's native combat HUD is attached to their spectator
  camera. Your original HUD returns with your own camera. Remote ammo and class
  gauge accuracy still need a live check; no extra network messages are sent.
- Offline play and split screen are left alone.
- Nothing is sent to other players. They do not need the plugin.
- There is no manual button to switch teammates yet.
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

## Package a release

Python 3 is required in addition to the build tools.

```sh
python3 -m pip install --require-hashes -r tools/requirements.txt
python3 tools/package.py
```

On Windows, install those requirements with `python -m pip` and run
`python tools/package.py`. Packaging builds the plugin and creates
`release/EDF6Spectate-<version>.zip` and its `.zip.sha256`. It downloads the pinned
official EDFModLoader and reproduces the same forwarder fix as EDF6Coop, checking
both the input and output hashes. No sibling checkout is needed.

See `docs/releasing.md` for the tagging and publishing procedure. Packaging does
not run tests, and there is no GitHub CI workflow.

## Licenses

`src/guard.h` comes from edf-coop-stable's `game/` folder, released into the public
domain (`third_party/EDF6Coop/LICENSE.txt`). `third_party/EDFModLoader/PluginAPI.h`
is MIT licensed (`third_party/EDFModLoader/LICENSE.txt`). This repository has no
chosen license yet. Loader packaging tools and Steam-library discovery were
adapted from EDF6Coop under MIT; see `third_party/EDF6Coop/TOOLS_LICENSE.txt`.
The release includes all three third-party notices.
