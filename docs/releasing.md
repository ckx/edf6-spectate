# Releasing

Keep experimental builds marked as prereleases until the live checklist in
`implementation-plan.md` is complete. Automated tests do not replace online play.

1. Fetch origin and integrate upstream changes. Update the version in
   `CMakeLists.txt`, the package readme and the README download link. Write
   `release-notes/<version>.md` with the actual validation status.
2. Run `tests/install_test.py` with Python. It executes both installers on Linux
   with PowerShell installed, or the Windows installer on Windows. These fixtures
   check fresh installation, upgrades, loader preservation, uninstall, invalid
   game paths and incomplete packages. They never use the actual game folder.
3. Install `tools/requirements.txt` with pip's `--require-hashes` option. Set
   `EDF6_GAME_DIR` to the game directory and run `python tools/package.py`.
   It builds and runs all four native tests with the game DLL, then verifies the
   ZIP contents and writes its checksum. On Linux, `SPECTATE_WINEPREFIX` can select
   a separate test prefix. Do not use the game's Proton prefix for tests.
4. Review the diff and commit. Push to `origin`, and wait for the Windows and
   installer CI jobs to pass. CI has no game binary and says which tests skipped.
5. Tag that commit `v<version>` and push the tag. Confirm that the packaged DLL
   matches that source tree; rerun packaging if any build input changed.
6. Publish the ZIP and checksum with the release notes. For the initial release:

   ```sh
   gh release create v0.1.0 \
     release/EDF6Spectate-0.1.0.zip release/EDF6Spectate-0.1.0.zip.sha256 \
     --repo ckx/edf6-spectate --verify-tag --prerelease \
     --title "EDF6 Spectate 0.1.0 experimental" \
     --notes-file release-notes/0.1.0.md
   ```

7. Download the published assets and compare the checksum with the local ZIP.

Only the release assets contain DLLs. Do not commit game files, builds, logs,
saves or local evidence. Packaging does not install anything in the game folder.

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
