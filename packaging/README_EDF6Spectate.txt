EDF6 Spectate 0.1.0 - experimental

Online behavior has not been tested. The automated Wine tests passed, including
tests against the supported EDF.dll. This does not verify a live online mission.

HOW IT WORKS
When you go down online, the camera follows a living teammate. If that player goes
down or leaves, it selects another. Reviving, or everyone going down, returns your
camera. Your HUD stays unchanged. Other players do not need this mod.
There is no manual player-switching button yet. Offline and split screen are left
alone. Supports the Steam EDF.dll from the January 2025 update only.

INSTALL ON WINDOWS
1. Close EDF6 and back up your saves.
2. Extract the whole ZIP to a separate folder.
3. Double-click INSTALL.bat. It finds EDF6 through Steam, or asks for its folder.
4. Start EDF6 normally from Steam.
If Windows asks about an unsigned downloaded script, review install.ps1 before
allowing it to run. The installer does not require administrator rights unless
your game folder itself requires them.

INSTALL ON LINUX / STEAM PROTON
1. Close EDF6 and back up your saves. Extract the whole ZIP.
2. In the extracted folder run: bash install.sh
   Or specify the game folder: bash install.sh "/path/to/EARTH DEFENSE FORCE 6"
3. Add WINEDLLOVERRIDES="winmm=n,b" to the game's Steam launch command.
   If the field is empty, use: WINEDLLOVERRIDES="winmm=n,b" %command%
   Merge with any existing DLL overrides and keep all your other launch options.

Both installers preserve an existing winmm.dll, all other plugins and settings.
The bundled EDFModLoader is installed only if winmm.dll is absent. No INI is shipped.
If ModLoader.ini already exists, its settings must permit loading plugins.
The installer does not back up saves automatically.

MANUAL INSTALL
Close EDF6. Copy Mods/Plugins/EDF6Spectate.dll into the matching game folder.
Copy winmm.dll and EDFModLoader_LICENSE.txt to the game root only if no loader is
already present. Keep the supplied license notices with the files.
Do not overwrite an existing loader by extracting the ZIP over your game folder.

CHECK / REMOVE
Mods/Plugins/EDF6Spectate.log should contain "Installed:" after launching.
"REFUSED:" means unsupported or modified game code; the plugin changes nothing.
"STOPPED:" means a runtime fault; stop testing and keep the log for a bug report.
Try going down, watching a teammate move, and getting revived. Online behavior,
disconnects, mission transitions and compatibility still need live verification.
Upgrade by running the newer installer with EDF6 closed.
Uninstall with UNINSTALL.bat, or: bash install.sh --uninstall
Only EDF6Spectate.dll is removed. The loader, other mods, logs and notices remain.

BUNDLED LOADER AND NOTICES
EDFModLoader v1.0.10 by BlueAmulet, MIT, includes the same forwarder fix used by
EDF6Coop. Each of its 180 forwarders uses its own target instead of shared state.
loader-fix.json records the changes. The fixed DLL SHA-256 is:
be94e1fac0ca12c41b6924e2eb168851641c999ce951d2a5a9faea5161b0f9a3
The package tools reproduce it from the hash-checked official release.
See EDFModLoader_LICENSE.txt, EDF6Coop_TOOLS_LICENSE.txt for adapted tooling and
installer code, and EDF6Coop_GUARD_LICENSE.txt for the public-domain memory guard.
No project-wide source license has been selected for Spectate.

Releases and bug reports:
https://github.com/ckx/edf6-spectate
