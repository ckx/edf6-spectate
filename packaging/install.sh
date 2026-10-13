#!/usr/bin/env bash
# Steam library discovery adapted from EDF6Coop, MIT; see EDF6Coop_TOOLS_LICENSE.txt.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
uninstall=0
if [ "${1:-}" = --uninstall ]; then uninstall=1; shift; fi
if [ "$#" -gt 1 ]; then echo 'Usage: bash install.sh [--uninstall] [game folder]' >&2; exit 1; fi

find_game() {
    local steam vdf lib candidate
    for candidate in "$here" "$(dirname "$here")"; do
        if [ -f "$candidate/EDF6.exe" ]; then echo "$candidate"; return; fi
    done
    for steam in "$HOME/.local/share/Steam" "$HOME/.steam/steam" "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam"; do
        vdf=$steam/steamapps/libraryfolders.vdf
        while IFS= read -r lib; do
            candidate=$lib/steamapps/common/EARTH\ DEFENSE\ FORCE\ 6
            if [ -f "$candidate/EDF6.exe" ]; then echo "$candidate"; return; fi
        done < <(printf '%s\n' "$steam"; if [ -f "$vdf" ]; then sed -n 's/^[[:space:]]*"path"[[:space:]]*"\(.*\)"/\1/p' "$vdf"; fi)
    done
    return 0
}

game=${1:-$(find_game)}
if [ -z "$game" ] || [ ! -f "$game/EDF6.exe" ]; then
    echo 'Could not find EDF6. Run: bash install.sh "/path/to/EARTH DEFENSE FORCE 6"' >&2
    exit 1
fi
if pgrep -ix 'EDF6.exe' >/dev/null; then
    echo 'Close EDF6 before installing or uninstalling.' >&2
    exit 1
fi
plugin=Mods/Plugins/EDF6Spectate.dll
if [ "$uninstall" = 1 ]; then
    rm -f "$game/$plugin"
    echo 'EDF6Spectate removed. The loader, other mods, and logs remain.'
    exit 0
fi
files=("$plugin" README_EDF6Spectate.txt EDF6Coop_GUARD_LICENSE.txt EDF6Coop_TOOLS_LICENSE.txt)
for file in "${files[@]}" winmm.dll EDFModLoader_LICENSE.txt; do
    if [ ! -f "$here/$file" ]; then echo "Missing $file. Extract the whole ZIP first." >&2; exit 1; fi
done
for file in "${files[@]}"; do
    if [ "$here/$file" -ef "$game/$file" ]; then continue; fi
    mkdir -p "$(dirname "$game/$file")"
    cp -f "$here/$file" "$game/$file"
done
if [ ! -e "$game/winmm.dll" ]; then
    cp -n "$here/EDFModLoader_LICENSE.txt" "$game/EDFModLoader_LICENSE.txt"
    cp -n "$here/winmm.dll" "$game/winmm.dll"
    echo 'Installed the bundled EDFModLoader.'
else
    echo 'Kept the existing winmm.dll.'
fi
echo 'EDF6Spectate installed. Automatic following only; no switching button yet.'
echo 'In Steam launch options, merge WINEDLLOVERRIDES="winmm=n,b" into your existing command.'
echo 'For an otherwise empty launch-options field: WINEDLLOVERRIDES="winmm=n,b" %command%'
echo 'Keep any existing DLL overrides and other options.'
