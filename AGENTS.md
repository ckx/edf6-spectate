# Working instructions

## Purpose and scope

Build a small EDFModLoader plugin that lets a downed player in an online room watch
a living teammate until revived. The accepted behavior is in `docs/design.md`.

Keep this independent of the sibling `edf-coop-stable` and `edf6-autoloot`
repositories. Do not require them at runtime. Use them as references for the loader
API, guarded reads, hooks and native-code test fixtures. Check licenses and preserve
required notices before copying source.

Do not add a free camera, watching while alive, offline or split-screen behavior,
network messages, HUD changes or unrelated patches without a new scope decision from
the user. The input for cycling teammates is an open decision (see the plan).

## Implementation

- Use C++20 and 4-space indentation. Keep both MSVC and clang/MinGW-w64 buildable.
- Windows and Proton are equally supported. Exercise Windows APIs under Wine.
- Prefer the game's own camera functions over writing camera or view fields. Prove
  their calling conventions and ownership first.
- Act on the main thread in the `System::Update` hook. Keep no game pointers between
  frames; find cameras and players again each frame.
- Never leave a camera current whose target has expired or left the room. The ghost
  camera's update reads a raw target pointer.
- Verify the game build and every byte the plugin relies on before patching. Refuse
  unsupported or conflicting code cleanly, with nothing changed.
- Do not use `__try`/`__except` for memory probes. Use `Guarded`/`ReadGuarded`
  (`src/guard.h`). A recovered fault does not undo a partial game call and is not
  permission to retry it.
- Keep hooks and logs small. Add helpers only when they enforce an invariant, manage
  lifetime or remove substantial repeated logic.

## Evidence and tests

`docs/research.md` records each address with its evidence. Distinguish observed
behavior from inference and keep unresolved questions.

Before adding a test, state the plausible defect, the production path it exercises
and the observable failure. Prefer the smallest fixture that reaches real code. Do not
reproduce the implementation in a mock, or test constants and source spelling.

Keep game binaries external, using `EDF6_GAME_DIR`. Tests requiring an absent game
binary report skipped; claims against game code must say when they were skipped.
Automated test success is not evidence of live behavior in an online room.

Live research may read the running game's memory; it must not write to it.

## Repository and delivery

Keep documentation aligned with implemented behavior and mark planned work clearly.
Do not add placeholder build commands or claim unperformed tests passed.
Keep game data, binaries, saves, logs and temporary output out of Git.
Use `local/` for persistent machine-specific evidence and `tmp/` for disposable work.

Inspect Git status before edits. Preserve unrelated work. Never discard commits or
uncommitted files to make an operation convenient. When an origin exists, fetch and
integrate upstream work before landing changes. Do not invent a remote destination.

Publish every completed build to this repository's GitHub Releases, including
experimental and diagnostic builds, so the user can distribute them to teammates.
Diagnostic tools also need a downloadable package; a local file or source commit
alone is not delivery. Commit and push the corresponding source and instructions,
use a unique release tag, upload the package and SHA-256 checksum, and download the
assets to verify them. State supported platforms, how to run the package, what was
tested and any unresolved behavior. Mark experimental and diagnostic releases as
prereleases. This is standing authorization to publish to the existing GitHub
origin without asking again. It does not authorize force pushes or installing
builds into the game.

Make commits for meaningful completed work. Use a lowercase imperative subject,
50 to 70 characters, with no prefix or trailing period. Add a short body only when
needed. Do not add AI attribution, co-author trailers or session links.

Close the game before changing its installation, back up saves before live testing,
and preserve the existing loader, other mods and Proton launch settings. Installing
experimental builds is a separate step from building.
