# EDF6 Spectate camera diagnostics 1

A read-only camera inspector for investigating spectator views that point in
the wrong direction. This release does not fix the reported camera problem or
change the installed spectator plugin.

Supported platform: Linux with EDF6 running under Proton. Requires Python 3.8 or
newer and the supported Steam EDF.dll, timestamp `0x678CCB46`. This inspector
does not run on Windows. The existing spectator plugin supports both platforms.

## Run the capture

Download `EDF6Spectate-camera-diagnostics-1.zip` from this release and extract it
to a separate folder. Leave the game and installed mods as they are. Open a
terminal in the extracted folder.

When the spectator view is wrong, run:

```sh
python3 inspect_camera.py "$(pgrep -n -x EDF6.exe)" --duration 30 > camera-direction.jsonl
```

This records 30 seconds and stops. Press Ctrl+C to stop sooner. If opening
`/proc/PID/mem` fails with `PermissionError`, run the same capture with permission
to inspect the process:

```sh
sudo python3 inspect_camera.py "$(pgrep -n -x EDF6.exe)" --duration 30 > camera-direction.jsonl
```

Send `camera-direction.jsonl` back with the teammate's class, whether they were
on foot or in a vehicle, and roughly when the view was wrong. A short recording
of the same interval helps match the numbers to what was visible.

The inspector records health, received look angles, character rotation and camera
direction. It opens game memory read-only. It does not install anything, modify
memory, send network messages or alter saves and launch settings.

## Validation and limits

The inspector successfully sampled both players and the current camera in a
running two-player online room under Proton. Earlier read-only captures totaling
90 seconds showed the remote character's rotation matching the received angles.
The local player stayed alive, so those captures did not reproduce the reported
spectator problem. Python syntax and command-line validation checks passed.

The game keeps running during sampling, so reads can straddle frames. A single
angular difference does not establish a fault. Some native camera modes can also
change the output direction. The inspector needs a capture during the bad view
to distinguish these cases.
