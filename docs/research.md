# Research notes

Static analysis of the supported EDF.dll on 2026-10-11, plus read-only sampling of a
running game in four-player online missions on the same date (two hours, three
missions). The sampler read
process memory through `/proc/<pid>/mem` under Proton and wrote nothing. The first
live run of 0.1.0 on 2026-10-12 crashed on entering spectate; see the correction
below. The user reported that 0.1.1 worked online. The log confirms following a
teammate and returning to the local camera. Full combat HUD support is new in
0.2.0 and has not yet been checked online.

## Supported image

`EDF.dll`, PE TimeDateStamp `0x678CCB46`, SizeOfImage `0x22CE000`, SHA-256
`0d5092393a1b9d8f1b75b0c93d8a8f65615a36a92c46e40349c2b66c733d0916`.
Image base `0x180000000`; all addresses below are RVAs.

The plugin checks the timestamp and size, then the FNV-1a 64 hash of each range
below. None of these ranges contains a base relocation, so the mapped bytes equal
the file bytes. The live process had the same bytes at the hook site and the called
functions. Any mismatch refuses installation before anything is written.

| Range | RVA | Size | FNV-1a 64 |
| --- | --- | --- | --- |
| `System::Update` call in `Application::Update` | `0x705715` | `0x5` | `0xF9744FB4304DB3F6` |
| `System::Update` | `0x1198DA0` | `0x9FB` | `0x7F0564B7A3BF2724` |
| CharacterGhostCamera factory | `0x82B40` | `0x1DC` | `0x6F6DB55CC4005B3C` |
| Camera combat HUD builder | `0x7FE5D0` | `0x988` | `0x30B0F56AE0009F97` |
| Normal player camera and HUD creation | `0x5A4F40` | `0x47C` | `0x43236D76F920E27E` |
| Camera HUD ownership transfer | `0x7FF390` | `0x9A` | `0x7B2D65E7F57F7623` |
| View camera switch | `0x1193FA0` | `0x18D` | `0xF859E0B97E448F2F` |
| `weak_from_this` | `0x1198950` | `0x9C` | `0x580BFED010D5E37E` |
| Camera flush | `0x1195A80` | `0x159` | `0xFAAA5CBBF9469555` |
| Current camera update | `0x11978D0` | `0x1000` | `0x15240CC9F660A4DA` |
| `create_remote_camera` debug command | `0xCCCA0` | `0x2D9` | `0x7B438E325C57D000` |
| CharacterGhostCamera constructor | `0xF4860` | `0x974` | `0xB9D5E9BDAA015326` |
| `IsPlayer` (SoldierBase slot 33) | `0x550890` | `0xA` | `0x1D923A0DEC9EF9A4` |
| Script `Object::IsAlive` | `0x1C0BE0` | `0x2E` | `0xA905EB899CAEB9D9` |
| Script `IsOnlineMode` | `0x1B5DF0` | `0x2B` | `0xA71A18502F48BC92` |
| `__RTDynamicCast` import thunk | `0x12DA7AA` | `0x6` | `0xD6E2BD7734AF1D4E` |

## Hook: the System::Update call

Observed:

- `Application` vtable slot 1 is `0x705530`, the per-frame update. At `0x705715` it
  runs `call 0x1198DA0` (`E8 86 36 A9 00`) with `rcx = *(EDF+0x20B2958)` (the
  `xgs::game::System`), `xmm1` a float step and `r8d` a count. Nothing else sets
  arguments for it. The signature is `void (System*, float, uint32)`.
- `System::Update` flushes cameras created since the last frame (`0x1195A80`, called
  at `0x1198F61`) before the scene steps.
- Later in the same frame, `0x7058B7` calls `0x11977F0`, which queues each view's
  current camera for drawing. That call sits behind a frame-skip branch at
  `0x70577D`; the `System::Update` call does not.

Hook: the plugin rewrites the call's rel32 to reach a stub next to EDF.dll
(`mov rax, imm64; jmp rax`) that enters the plugin. The plugin does its work and then
calls `0x1198DA0` with the same arguments. The return address stays in
`Application::Update`, so nothing unwinds through the stub.

Thread: the main thread, once a frame. Inference: game objects step inside
`System::Update` (the loop over `System+0xCC0` at `0x1198EA0`), so before the call the
scene is between steps. Job threads were not examined.

## Cameras

Observed:

- `System` has four views of `0x188` bytes from `+0x4D0`. A view holds its current
  camera as a `weak_ptr` at `+0x8` (object) and `+0x10` (control block), and its
  cameras as a `std::list<shared_ptr<SceneCamera>>` at `+0x18` (sentinel pointer;
  size at `+0x20`; node: next `+0`, value `+0x10`). `0x1195BE0` and `0x1196850`
  index views this way.
- Live: online, view 0 held one camera, a `CharacterGhostCamera` (vtable
  `0x1768C10`) following the local player. Views 1 to 3 were empty.
- Player creation (`0x5A4F40`) builds the player and a `CharacterGhostCamera` for it
  through the factory `0x82B40`, with the view index equal to the player index.
- Factory `0x82B40(System* rcx, weak_ptr* out rdx, uint32 view r8d,
  const Matrix* r9, InitParam* [rsp+0x20])`: fills the InitParam's base fields
  (matrix at `+0x10`, scene at `+0x50`, view at `+0x58`, viewport at `+0x5C`, owner
  output shared-pointer address at `+0x70`), allocates `0xC20` bytes, runs the constructor, queues the camera on
  `System+0x4C0` (`0x1195990`, under the lock at `System+0x308`) and returns a
  `weak_ptr<CharacterGhostCamera>` in `out`. Matrix and InitParam must be 16-byte
  aligned (`movaps`).
- The InitParam vtable is `0x1762168` (`CharacterGhostCamera::InitParam`). Its
  `+0x80` is the object to follow. Callers fill only the vtable and `+0x80`; the
  game's own InitParams are `0x88` bytes on the stack.
- The constructor `0xF4860` copies the target's `weak_ptr` (from the target's
  `enable_shared_from_this` at `+0x28`/`+0x30`) into the camera's `+0x350`/`+0x358`.
  It holds no strong reference. `+0x360` caches a raw `SoldierBase*` from the same
  object.
- The ghost camera's update, slot 3 (`0xF8210`), locks the target `weak_ptr`
  (`0xF828C`) but then uses `+0x360` with only a null check (`0xF82A4`). A camera
  whose target was destroyed would read freed memory there.
- `0x11978D0` runs slot 3 on each view's current camera only (`0x11979B4`). Outside
  debug commands, its one caller is `System::Update` at `0x1198FA4`, after the camera
  flush and before the scene steps. A camera that is not current is not updated,
  and an object destroyed during the scene steps is seen as expired by the plugin
  at the start of the next frame, before the next camera update. The plugin moves
  view 0 off a ghost camera whose target expired or left the room.
- The flush `0x1195A80` adds each queued camera to its view's list and makes it the
  view's current camera through `0x1193FA0`. A new camera is shown on the frame
  after it is created.
- `0x1193FA0(View*, weak_ptr<SceneCamera> by value)`: calls slot 7 (`+0x38`) on the
  old current camera, stores the new one, calls slot 6 (`+0x30`) on it, and releases
  its by-value argument (the weak count decrement at `0x1194109`). Callers build the
  argument with `weak_from_this` (`0x1198950`), as `0x11932F0` does. The camera test
  runs these two functions from EDF.dll and checks the reference counts.
- The developers' debug command `create_remote_camera <id>` (`0xCCCA0`, registered at
  `0xCCB1F`) finds the remote player with that slot in the TeamManager map and creates
  a `CharacterGhostCamera` for it on view 0 through the same factory. This is the
  evidence that the ghost camera supports remote players. `next_camera`
  (`0x9F5B0`) cycles a view through its list with `0x1196850`.
- No game code removing one camera from a view's list was found. Lists are emptied
  when the System tears down (`0x11956C0`). Cameras the plugin creates stay until
  then; the plugin creates at most one per teammate per downed period and reuses
  them.
- Only debug commands call `0x1196850` (cycle cameras) and `0x11988D0` (visit every
  camera), so extra cameras in view 0's list do not change normal play.

## Players

Observed:

- `*(EDF+0x20B2978)` is the `TeamManager`. `[TeamManager+0x38]` points at a
  `std::map` whose values are the players' objects (node: left `+0`, parent `+8`,
  right `+0x10`, nil flag `+0x19`, object `+0x20`). `create_remote_camera` walks it
  from the head's left node and casts each object with
  `__RTDynamicCast(object, 0, GameObjectBase 0x2006400, SoldierBase 0x2006428, 0)`.
  Live, the map held exactly the four players (three `PaleWing`, one `HeavyArmor`).
  Missions with NPC allies were not sampled.
- `IsPlayer` (`0x550890`): `+0x340` (local controller) or `+0x1ED0` (network player
  record) is non-null. Live: the local player had `+0x340` set and `+0x128` bit 0
  clear; the three remote players had `+0x340` null and `+0x128` bit 0 set. Every
  player had a record at `+0x1ED0` whose `+0x48` was its slot (local 0, remotes 1
  to 3).
- Health is the float at `+0x2F8`; `+0x2F4` held a constant per-player value larger
  than every observed health (inference: maximum armor). The script's
  `Object::IsAlive` (`0x1C0BE0`) is `health > 0`.

### Downed state

Observed in 2.5 minutes of sampling at 2 Hz: the local player (slot 0) went down at
02:35:45 and was back at 02:35:53 with health 54.4 of 190.6; slot 1 went down at
02:36:28 and was back at 02:36:47 with 13.6. While down, the same object stayed in
the map, health was exactly 0, and these fields differed from every sample while up:

| Field | Down | Up |
| --- | --- | --- |
| `+0x2F8` health | `0.0` | `> 0` |
| `+0x2E8`, `+0x2E9` (bytes) | `1`, `1` | `0`, `0` |
| `+0x1180` | `1` | `0` |
| `+0x194` (byte) | `1` | `0` |

The local player's camera stayed the same object throughout. Its `+0x3D4` byte
followed the target's `+0x194` (the constructor copies `+0x170..` or `+0x230..` of the
target into `+0x3B0..`).

The plugin treats `health <= 0` as down, as `IsAlive` does. Over the full two-hour
sample (three missions, 02:35 to 03:08, then the game closed), there were 12 downed
periods across slots 0, 1 and 2, each ending in a revive after 8 to 20 seconds.
`+0x1180` and `+0x194` matched the downed state in every sample for slots 0 and 2;
`+0x2E8` did not hold across the whole sample. No player was seen staying down
until the mission ended, and nobody left the room.

### End of a mission

About 12 seconds before each mission unloaded (02:38:40, 02:54:09, 03:06:36), a
second set of four players appeared in the TeamManager map, one more object per
slot, with full health and the same local and remote flags. The game then created a
second `CharacterGhostCamera` on view 0 following the new local object, and it became
current. Health of the new remote objects changed while the old ones stayed
constant. Then every view and player was gone until the next mission started.

The plugin does nothing while any slot has two players, so it never moves the view
off the game's new camera.

## Online mode

`IsOnlineMode` (`0x1B5DF0`) reads the session mode table at `*(EDF+0x20B2890)`. See
the AutoLoot research notes for the layout. The live sample returned online.

## First live crash and ownership correction

On 2026-10-12, the user went down in an online mission and the game immediately
closed. The spectator log recorded `Following the player in slot 1 (new camera)`
at 19:04:55. The co-op crash log recorded an access violation at `EDF+0x11940CD`
at 19:04:55.621, with callers `0x1195B81`, `0x1198F66`, and Spectate's update hook.
The failing instruction is the new camera's activation call through vtable slot 6.
The recorded vtable address was in the heap, rather than EDF.dll. Logs and the
installed binary are preserved under `local/crash-20261013T020534Z/` and are not
committed.

The initial factory return-type analysis above was wrong. Rechecking its code:

- `0x82C69` and `0x82C6C` write the object and control block to the return buffer.
- `0x82C70` increments the control block's weak count at `+0xC`.
- `0x82C76` releases the temporary strong reference at `+0x8`.
- The native debug command releases this result at `0xCCF2E`, decrementing only
  `+0xC` and calling the block-deletion function at vtable `+0x8` if it reaches zero.

Version 0.1.0 instead decremented the strong count after the factory returned.
That can destroy the queued camera before the flush activates it. This is a
confirmed ownership bug and explains the observed invalid activation call; no
memory dump was available to inspect the destroyed object directly.

Version 0.1.1 releases only the returned weak reference, matching the native caller.
The factory and caller are already in the plugin's verified code ranges. The user
reported a successful online retry. The installed 0.1.1 log recorded following
slot 1 at 19:34:30, returning to the player's own camera at 19:35:43, and another
spectate at 20:01:05 on 2026-10-12. This confirms basic following and return, not
the complete disconnect, class and mission-transition checklist.

## Combat HUD attached to the spectator camera

Scope: the user requested the full teammate combat HUD after the successful
camera retry. Version 0.2.0 uses the native player-HUD builder, with no additional
network messages or new overlay.

Observed in the supported game code:

- Normal player creation `0x5A4F40` calls the camera factory at `0x5A5278`, locks
  its returned weak reference and takes the camera object into `r15` at
  `0x5A52B0`. It then calls `0x7FE5D0` at `0x5A5306` with `rcx = camera` and
  `rdx = player`. The return value is unused.
- `0x7FE5D0(SceneCamera*, GameObjectBase*)` casts the player to SoldierBase and
  builds the HUD objects through eight native factories. These include
  `HuiHudWeaponObject`, the health/power gauge and radar UI objects, crosshairs,
  damage display, follower durability, rescue text, multiplayer status and chat.
  It reads the camera's view index at `+0x60` for the UI layout.
- The weapon HUD factory `0x7FDA40` puts the camera into the InitParam at `+8`.
  After constructing the HUD, it moves a strong reference into the camera's HUD
  list at `+0x208` through `0x7FF390`, called at `0x7FDB64`. That helper moves the
  object/control pair into a list node and zeros the source pair. The wrapper's
  return value is a weak reference, which the builder releases at `+0xC`.
- `HUiHudBase` construction at `0x815810` copies the selected player's weak
  reference into the widget's `+0x778/+0x780`. Weapon update `0x831A40`, power
  gauge update `0x826250` and radar update `0x82B4D0` lock that reference before
  reading the player. The weapon constructor chooses its layout and weapon slots
  from that player. Rebinding the local widget alone would not rebuild that layout.
- The HUD wrapper stores its camera at `+0x18`, populated by `0x11D4320`.
  `0x8168B0` checks that camera's visibility and enables its UI widgets for update.
  HUD drawing and lifetime stay under the game camera's existing ownership.
- None of the three additional verified code ranges has PE base relocations.

A read-only `/proc/<pid>/mem` sample of the running 0.1.1 session found the current
local camera with 11 HUD objects and the existing teammate camera with zero. Both
were CharacterGhostCamera instances. The sample is stored in
`local/hud-research/live-camera-huds.json`; the running process was not modified.

Implementation: after creating a teammate camera, call `0x7FE5D0` once with that
camera and its target, before releasing the factory's weak reference. The queued
camera still has the System's strong owner. Reusing the camera reuses its HUD.
The original local camera and its widgets are untouched, so returning to that
camera also returns its HUD through the normal camera path.

Still unverified: remote ammo/reload and class-gauge accuracy, all class layouts,
crosshair behavior, and HUD cleanup after a teammate disconnect or mission end.
This reads the game's remote-player copy; it does not replicate any state the
game itself does not send. A live HUD check is required.

## Open questions

- Live combat HUD check, including remote ammo, reloads and class gauges. The basic
  0.1.1 camera follow and return were confirmed by the user and log.
- Whether a player can stay down until the mission ends, or bleeds out, and what
  happens to the object then.
- Whether NPC allies appear in the TeamManager map. The plugin only takes objects
  with a network player record and the remote bit, so NPCs would be skipped.
- Whether a remote player's object is destroyed when that player leaves the room,
  and whether anything other than the scene steps destroys it between the plugin's
  check and the camera update.
- Whether any job thread reads view 0's camera while `Application::Update` runs.
- Which input should cycle teammates. The prototype has none.
