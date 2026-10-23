# Implementation plan

## M1. Research (done)

Static analysis of the camera system, the player map and the downed state, and a
read-only sample of a live four-player room. Evidence is in `research.md`.

## M2. Automatic follow prototype

The `System::Update` call hook, the roster read, `NextTarget`, camera reuse and
creation through the game's factory, and refusal on any unexpected byte.

Automated checks, all passing under Wine with the game's EDF.dll:

- `TargetChoice`: whom a downed player watches.
- `CameraSwitchWithGameCode`: the game's `weak_from_this` and view switch move a view
  between two cameras with balanced reference counts.
- `PluginRefusesOutsideGame`, `PluginLoadsIntoGame`: refusal without EDF.dll, and the
  call site leading into the plugin with it.

Camera creation, the roster walk and the live hand-over cannot run outside the game
(the factory allocates through the game's heap and builds scene objects).

## M3. Live check in an online room (partly verified)

The first live attempt with 0.1.0 crashed when activating the new camera. Version
0.1.1 corrects the weak-reference release, as documented in `research.md`. The
user confirmed that 0.1.1 worked online. Its log recorded a camera switch at
19:34:30 and a return at 19:35:43 on 2026-10-12. This establishes basic following
and return, not the complete checklist below.

Install `out/EDF6Spectate.dll` into `Mods/Plugins` with the game closed, back up
saves, and play an online mission with at least two other players. Check:

1. The log says `Installed:` and nothing about refusal.
2. Going down switches the view to a standing teammate within a frame or two, and the
   log names the slot. The view follows their movement and aim.
3. When that teammate goes down, the view moves to the next one.
4. Being revived returns the view to the player's own camera, and aiming, zoom and
   the HUD behave as before.
5. Going down a second time in the same mission reuses the cameras (no `new camera`
   line for a slot already watched).
6. A teammate leaving the room while watched: the view moves on, and the game keeps
   running.
7. Mission clear and mission failure while watching: results and the next mission
   start normally.
8. Offline play and split screen: no log lines after `Installed:`.

Record the results, with the log, in `research.md`. A green test run does not stand in
for any of these.

## M4. Choosing whom to watch (needs a decision)

A button to cycle teammates while down. Decide the input with the user first: the
game's own input state is the most consistent with rebinding, XInput or keyboard
polling are simpler. Add it only after M3 passes.

## M5. Release packaging

The experimental 0.1.0 package includes Windows and Linux installers, a bundled
EDFModLoader for fresh installs, notices and a SHA-256 file. Existing loaders and
other mods are preserved. Packaging rebuilds the plugin and creates the ZIP.

The user requested an installable release before live testing. Publish it as an
experimental prerelease with the missing live checks stated. M3 is still pending;
a stable release follows successful live verification. See `releasing.md`.

## M6. Spectated teammate's health and names

The user requested the full combat HUD after the successful 0.1.1 camera retry.
Version 0.2.0 attached the native local-player HUD builder to each teammate camera.
The online attempt crashed in the damage HUD after setup had already faulted.
Version 0.2.1 removes that attachment and restores the working camera-only path.

Before another HUD build, separate the viewed teammate's combat state from the
local viewer's settings. Damage, radar and weapon widgets read a local-player
index that is invalid for remote players. Check constructor and update paths,
including indirect calls, before enabling any widget. A caught construction fault
cannot roll back registered objects. Evidence and exact addresses are in
`research.md`.

After those dependencies are resolved, live checks still needed: health, ammo and
reload changes; different classes and weapon layouts; radar and crosshair; return
on revival; repeated spectating; disconnects and mission transitions. Remote
values are only as current as the game's own copy of that teammate.

The user narrowed the next attempt to health and remote names after the crash.
Version 0.3.0 calls only the health-layout factory and the multiplayer indicators
factory. The watched player owns the displayed health values; the local player
remains the name indicators' viewer. The native enumeration excludes that viewer,
not the watched teammate. Full HUD replication is deferred.

The health constructor, update/display methods and their player-data helpers were
inspected against EDF.dll. The multiplayer name path uses the player's network
record and room slot. New code checks cover those interfaces. The release is built
and packaged, but no live HUD check has passed yet. Check health changes, name
visibility, inactive-camera HUD hiding, revival, a second death, vehicles and
teammate departures before expanding the HUD.

## M7. Orbit camera aimed by the downed player

The user asked for a free camera instead of the teammate's own view and chose an
orbit around the watched teammate, aimed with the downed player's normal look
controls (2026-10-22). A detached free-fly camera is out of scope.

Version 0.4.0 sets the watched remote soldier's camera parameters to the ghost
camera's orbit mode with the local player's look pitch and yaw each frame, and
clears it on every other remote soldier. Evidence and the reasoning about the HUD
are in `research.md` (Orbit camera). Built and run against the existing tests; no
live check has passed.

Live checks:

1. Going down: the view moves to the teammate and then turns with mouse and right
   stick. Up/down limits match normal aiming.
2. The camera stays near the teammate as they move, with no clipping or jitter
   beyond the normal follow camera.
3. A target change keeps the current look direction.
4. Revive: the player's own camera returns facing the last look direction, with
   the normal HUD and aim.
5. Whether the health gauge hides while orbiting (predicted) and whether the
   name/health indicators stay.
6. Watched teammates of each class, on foot and in vehicles.
