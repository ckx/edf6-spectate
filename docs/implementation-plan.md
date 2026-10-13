# Implementation plan

## M1. Research (done)

Static analysis of the camera system, the player map and the downed state, and a
read-only sample of a live four-player room. Evidence is in `research.md`.

## M2. Automatic follow prototype (done, not played)

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

## M3. Live check in an online room (next)

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
