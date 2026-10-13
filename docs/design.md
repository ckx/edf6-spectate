# Design

## Goal

In an online room, a player who is down and waiting for a revive watches a living
teammate through the game's own follow camera instead of looking at their own body.
When they are revived, or nobody is left to watch, the view returns to their own
camera.

## Behavior

- Online only. Offline play, including split screen, is never touched.
- When the local player's health reaches 0 while their character stays in the room
  (the downed state), view 0 switches to a camera following the teammate with the
  lowest slot who is not down.
- The followed teammate stays followed while they stand. When they go down or leave,
  the view moves to the next standing teammate in slot order, wrapping round.
- When the local player is revived, the view returns to their own camera. It also
  returns when every teammate is down.
- A camera following someone who left the room is never left current.
- While the game holds two sets of players at the end of a mission, the plugin does
  nothing, so the game's own camera change goes through.
- Nothing is sent over the network. Each player's plugin acts on its own view only.
  Players without the plugin see no difference.
- The spectator camera gets the followed teammate's native combat HUD, including
  health, weapons, ammo, class gauges, radar and crosshair. It reads the game's
  local copy of that teammate. No additional state is sent over the network.
- Returning to the local camera also returns its original HUD. This replaces the
  previous camera-only scope at the user's request. HUD behavior still needs a
  live check, including whether remote ammo and class gauges stay accurate.

Not in the prototype: a button to choose whom to watch, a free camera, watching while
alive, and offline split screen. A cycle button is the next decision for the user
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
5. When the target differs, it makes an existing camera in view 0's list that follows
   the target current (`weak_from_this` and the view switch, as the game's
   `0x11932F0` does), or, for a teammate without one, creates a camera through the
   game's factory with the same InitParam `create_remote_camera` builds. The game's
   flush in the `System::Update` that follows makes the new camera current.
6. For a newly created teammate camera, before releasing the factory's weak
   reference, attach its player's HUD with `0x7FE5D0`, as normal player creation
   does at `0x5A5306`. The camera owns the HUD objects in its list at `+0x208`.
   Reusing that camera also reuses its HUD; no local-player binding is changed.

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
following a teammate and returning to the player's own camera. The new HUD path
in 0.2.0 has not yet been verified online.

Every read and call runs in `Guarded`. A fault stops the plugin for the rest of the
session and logs it; the game call is not retried.

## Installation and refusal

`EML6_Load` checks the image timestamp and size and the FNV-1a 64 hash of every
range listed in the research notes, including the call site. It allocates a stub
within rel32 reach of EDF.dll, then rewrites the call's displacement. Any failure
before the write refuses with nothing changed. The plugin then stays loaded.

## Compatibility

- EDF6Coop: no shared code sites. Its player sync and hit authority do not touch
  cameras. The user reported successful camera following with 0.1.1 in the local
  installation, which also has EDF6Coop. HUD compatibility remains unverified.
- Other mods that change any hashed range make the plugin refuse.
- Every hashed range matched in the running game on 2026-10-11, with EDF6Coop and
  Lootmaster's Patcher installed.

## Open decisions

- Input to cycle teammates or return to the own camera while down, and how to read it
  (game input, XInput, keyboard).
- Whether to watch while the local player has bled out, if the game has such a state.
- Whether to show the followed teammate's name.
