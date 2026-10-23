# Design

## Goal

In an online room, a player who is down and waiting for a revive watches a living
teammate through the game's own follow camera instead of looking at their own body.
The camera orbits that teammate and points where the downed player looks, so they aim
it with their normal look controls. When they are revived, or nobody is left to
watch, the view returns to their own camera.

## Behavior

- Online only. Offline play, including split screen, is never touched.
- When the local player's health reaches 0 while their character stays in the room
  (the downed state), view 0 switches to a camera following the teammate with the
  lowest slot who is not down.
- The followed teammate stays followed while they stand. When they go down or leave,
  the view moves to the next standing teammate in slot order, wrapping round.
- When the local player is revived, the view returns to their own camera. It also
  returns when every teammate is down.
- The spectator camera orbits the watched teammate at the downed player's own look
  pitch and yaw, as set by mouse or right stick through the game's bindings. It does
  not copy the teammate's aim. Version 0.4.0 adds this; it is unverified online.
- A camera following someone who left the room is never left current.
- While the game holds two sets of players at the end of a mission, the plugin does
  nothing, so the game's own camera change goes through.
- Nothing is sent over the network. Each player's plugin acts on its own view only.
  Players without the plugin see no difference.
- Version 0.3.0 attaches only a health gauge and multiplayer name/health indicators.
  The gauge reads the watched teammate's current/maximum health directly. The name
  indicators keep the local player as their viewer, so their normal exclusion of
  the viewer does not hide the watched teammate. Names use the game's settings.
- Returning to the local camera returns its original HUD. No ammo, radar, damage
  or class gauges are created for spectator cameras. The smaller HUD is unverified
  online. Full combat HUD support is deferred in favor of health and names.

Not in the prototype: a button to choose whom to watch, a detached free-fly camera,
watching while alive, and offline split screen. A cycle button is the next decision for the user
(see the plan).

## Runtime design

The plugin takes the `System::Update` call in `Application::Update` (`0x705715`).
Each frame, before calling the game's `System::Update`, it:

1. Returns unless `IsOnlineMode()` is true and view 0's current camera is a
   `CharacterGhostCamera`.
2. Reads the players from the TeamManager map, as the game's `create_remote_camera`
   debug command does: the local player (local controller, not remote) and the
   remote players (remote bit, network record), with slot and health. It returns if
   any slot has two players.
3. Works out whom the current camera follows: the local player, a teammate by slot,
   or someone gone.
4. Chooses the target with `NextTarget` when the local player is down, else the
   local player's own camera.
5. Sets the target teammate's camera parameters to orbit at the local player's look
   angles, and clears that setting on every other remote player (see Orbit camera).
6. When the target differs, it makes an existing camera in view 0's list that follows
   the target current (`weak_from_this` and the view switch, as the game's
   `0x11932F0` does), or, for a teammate without one, creates a camera through the
   game's factory with the same InitParam `create_remote_camera` builds. The game's
   flush in the `System::Update` that follows makes the new camera current.
7. For a new camera, attach the health widget through `0x7FC9A0` using only the
   `lyt_HudPowerGauge01.sgo` layout and the watched player. Attach multiplayer
   indicators through `0x7FD580` using the local player as the viewer. Release
   both returned weak references; the camera owns the objects. The full HUD builder
   at `0x7FE5D0` is never called. Invalid health/max values skip the health widget.

The plugin keeps no game pointers between frames. Its only state is a bit per slot
for cameras it asked for during the current downed period, so a camera that fails to
appear is not requested every frame. Cameras it creates stay in view 0's list until
the System tears the views down at the end of the mission, and are reused if the
player goes down again. The game updates only a view's current camera, so the extra
cameras cost nothing while unused.

The camera factory returns a weak reference; the System's queue owns the camera.
The plugin releases only that weak reference. Version 0.1.0 incorrectly released a
strong reference and crashed on its first live camera activation. Version 0.1.1
corrects this. The user confirmed that 0.1.1 worked online; its log also recorded
following a teammate and returning to the player's own camera. Version 0.2.1
restores that camera-only path after the HUD addition in 0.2.0 crashed online.
Version 0.3.0 adds the two isolated HUD components described above.

Every read and call runs in `Guarded`. A fault stops the plugin for the rest of the
session and logs its exception code and instruction address; the game call is not
retried. This cannot undo partial game
object creation or make those objects safe for later native updates.

## Orbit camera

A `CharacterGhostCamera` copies a camera parameter block from its target each update,
before its follow state computes the view. Byte `+0x40` of that block switches the
follow state from the target's own facing to an orbit at the pitch/yaw pair at
`+0x50`, with the target's normal offsets, smoothing and position. The game never sets
this byte to anything but 0 on a soldier, so the plugin owns it. Each frame, before
`System::Update`, it writes the downed local player's look pitch (`+0x1230`) and yaw
(`+0x1234` plus `+0x1E44`, as the network sends it) into both blocks of the watched
remote soldier and sets the byte. Every other remote soldier with the byte set gets 0
back. The local player's own soldier and camera are never written.

No game function sets these fields, so the plugin writes them. The writes touch only
the local copy of a remote player. The only readers found are the ghost camera's
constructor and update, and class updates that run for local players. Only the
plugin's spectator cameras follow remote players. Nothing is sent to the network. A parameter reload the
game runs inside `System::Update` (on a revive or some state changes) clears the byte
for one frame, after which the plugin sets it again.

Inference from static analysis: a ghost camera in orbit mode reports through vtable
slot 9 that HUD widgets should hide. The health gauge checks that slot, so it is
expected to be hidden while orbiting. The multiplayer name/health indicators do not
call it. Whether the downed player's look input keeps working while the spectator
camera is in orbit mode, and how the HUD behaves, are live checks.

## Installation and refusal

`EML6_Load` checks the image timestamp and size and the FNV-1a 64 hash of every
range listed in the research notes, including the call site. It allocates a stub
within rel32 reach of EDF.dll, then rewrites the call's displacement. Any failure
before the write refuses with nothing changed. The plugin then stays loaded.

## Compatibility

- EDF6Coop: no shared code sites. Its player sync and hit authority do not touch
  cameras. The user reported successful camera following with 0.1.1 in the local
  installation, which also has EDF6Coop. The unsafe HUD attachment is removed.
- Other mods that change any hashed range make the plugin refuse.
- Every hashed range matched in the running game on 2026-10-11, with EDF6Coop and
  Lootmaster's Patcher installed. The ranges added in 0.4.0 matched the running
  game on 2026-10-22.

## Open decisions

- Input to cycle teammates or return to the own camera while down, and how to read it
  (game input, XInput, keyboard).
- Whether to watch while the local player has bled out, if the game has such a state.
- Whether a separate fixed spectator-name caption is needed beyond the native
  multiplayer indicators.
