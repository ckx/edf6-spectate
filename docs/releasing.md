# Releasing

Keep experimental builds marked as prereleases until the live checklist in
`implementation-plan.md` is complete. Automated tests do not replace online play.

Every completed build, including diagnostics, must have downloadable GitHub release
assets. Publish the corresponding source and instructions too. AGENTS.md provides
standing authorization for publishing to the existing origin.

1. Fetch origin and integrate upstream changes. Update the version in
   `CMakeLists.txt`, the package readme and the README download link. Write
   `release-notes/<version>.md` with the actual validation status.
2. Install `tools/requirements.txt` with pip's `--require-hashes` option and run
   `python tools/package.py`. It builds the plugin, verifies the ZIP contents and
   writes its checksum. It does not run tests or require a game installation.
3. Review the diff, commit and push to `origin`. There is no CI workflow.
4. Tag that commit `v<version>` and push the tag. Confirm that the packaged DLL
   matches that source tree; rerun packaging if any build input changed.
5. Publish the ZIP and checksum with the release notes. For the initial release:

   ```sh
   gh release create v0.1.0 \
     release/EDF6Spectate-0.1.0.zip release/EDF6Spectate-0.1.0.zip.sha256 \
     --repo ckx/edf6-spectate --verify-tag --prerelease \
     --title "EDF6 Spectate 0.1.0 experimental" \
     --notes-file release-notes/0.1.0.md
   ```

6. Download the published assets and compare the checksum with the local ZIP.

Only the release assets contain DLLs. Do not commit game files, builds, logs,
saves or local evidence. Packaging does not install anything in the game folder.

## Standalone diagnostics

Diagnostic tools that do not change the plugin use their own release tags and
packages. Keep the plugin version unchanged. For the first camera inspector,
use tag `camera-diagnostics-1` and package `EDF6Spectate-camera-diagnostics-1.zip`.
The ZIP contains `tools/inspect_camera.py` as `inspect_camera.py` and
`release-notes/camera-diagnostics-1.md` as `README.md`. Include its SHA-256 file
as a separate release asset. Publish as a prerelease, then download both assets
and verify the checksum and contents against the local package.

## Bundled loader

`tools/fetch_modloader.py` and `tools/fix_winmm_proxy.py` were adapted from
EDF6Coop under its MIT license, preserved in
`third_party/EDF6Coop/TOOLS_LICENSE.txt`. The fixer uses pefile directly rather
than pulling in the sibling project's general disassembler. The resulting DLL
must match its verified fixed-loader SHA-256. `loader-fix.json` in the ZIP records
every changed forwarder. The loader's own MIT license is also included.

An existing loader is always preserved by these installers. A fresh install gets
the bundled loader. A user with a conflicting proxy or an older loader must review
that setup separately; Spectate's installer never replaces it automatically.
