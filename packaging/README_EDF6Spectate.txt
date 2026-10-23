EDF6 Spectate 0.4.0 - experimental

Version 0.4.0 makes the spectator camera orbit the watched teammate. You aim it
with your own look controls instead of seeing the teammate's view. This has not
been checked online yet. Version 0.3.0 is the fallback that copies the teammate's
view.

HOW IT WORKS
When you go down online, the camera follows a living teammate. If that player goes
down or leaves, it selects another. The camera orbits that teammate and points
where you look: turn it with the mouse or right stick, using your normal bindings
and sensitivity. Reviving, or everyone going down, returns your
camera and original HUD. The spectator health bar reads the watched teammate's
current and maximum health; the game may hide this bar while the camera orbits.
Multiplayer name/health indicators remain bound to
you as the viewer so the watched player stays included. Name display follows the
game's normal settings. Ammo, radar, damage effects and class gauges stay disabled
on spectator cameras. Other players do not need this mod. No extra network
messages are sent.
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
Try going down, looking around your teammate with the mouse or right stick, and
getting revived. Check whether the health bar and the name/health indicators show,
and that your own camera and HUD return after revival. Online behavior,
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
