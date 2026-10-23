# EDF6 Spectate (moved to EDF6Coop)

The spectator camera is part of EDF6Coop from version 3.1.0, so one install does
both. Use EDF6Coop:

https://github.com/ckx/edf-coop-stable/releases/tag/v3.1.0

There it is on by default; `[Display] SpectateWhenDown=0` in `EDF6Coop.ini` turns
it off. EDF6Coop 3.1.0 and later rename an installed `EDF6Spectate.dll` to
`.disabled`, since both would take the same game call. Do not install the two
together.

In the EDF6Coop repository the source is `game/spectate.*`, the design, evidence
and live checks are in `docs/spectate/`, and the read-only camera inspector is
`tools/inspect_camera.py`.

This repository keeps the history of EDF6Spectate 0.1.0 to 0.4.0 and the
camera-diagnostics release. No further changes land here.
