param([switch]$Uninstall, [string]$GameDir = '')
# Steam library discovery adapted from EDF6Coop, MIT; see EDF6Coop_TOOLS_LICENSE.txt.
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot

function Find-GameDir {
    if ($GameDir) { return $GameDir }
    foreach ($folder in @($here, (Split-Path -Parent $here))) {
        if ($folder -and (Test-Path -LiteralPath (Join-Path $folder 'EDF6.exe') -PathType Leaf)) { return $folder }
    }
    $roots = @()
    foreach ($key in 'HKCU:\Software\Valve\Steam', 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam', 'HKLM:\SOFTWARE\Valve\Steam') {
        try {
            $steam = Get-ItemProperty -LiteralPath $key -ErrorAction Stop
            foreach ($path in $steam.SteamPath, $steam.InstallPath) { if ($path) { $roots += $path } }
        } catch {}
    }
    foreach ($root in ($roots | Select-Object -Unique)) {
        $libraries = @($root)
        $vdf = Join-Path $root 'steamapps/libraryfolders.vdf'
        if (Test-Path -LiteralPath $vdf) {
            foreach ($match in [regex]::Matches((Get-Content -LiteralPath $vdf -Raw -Encoding UTF8), '"path"\s+"([^"]+)"')) {
                $libraries += $match.Groups[1].Value.Replace('\\', '\')
            }
        }
        foreach ($library in $libraries) {
            $folder = Join-Path $library 'steamapps/common/EARTH DEFENSE FORCE 6'
            if (Test-Path -LiteralPath (Join-Path $folder 'EDF6.exe') -PathType Leaf) { return $folder }
        }
    }
    return ''
}

$game = Find-GameDir
if (-not $game) {
    Write-Host 'In Steam, right-click EDF6 -> Manage -> Browse local files.'
    $game = (Read-Host 'Paste that game folder path').Trim('"', ' ')
}
if (-not $game -or -not (Test-Path -LiteralPath (Join-Path $game 'EDF6.exe') -PathType Leaf)) {
    throw 'No EDF6.exe in that folder.'
}
if (Get-Process -Name EDF6, EDF6.exe -ErrorAction SilentlyContinue) {
    throw 'Close EDF6 before installing or uninstalling.'
}
$plugin = 'Mods/Plugins/EDF6Spectate.dll'
if ($Uninstall) {
    $target = Join-Path $game $plugin
    if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Force }
    Write-Host 'EDF6Spectate removed. The loader, other mods, and logs remain.'
    return
}
$files = @($plugin, 'README_EDF6Spectate.txt', 'EDF6Coop_GUARD_LICENSE.txt', 'EDF6Coop_TOOLS_LICENSE.txt')
foreach ($file in ($files + @('winmm.dll', 'EDFModLoader_LICENSE.txt'))) {
    if (-not (Test-Path -LiteralPath (Join-Path $here $file) -PathType Leaf)) {
        throw "Missing $file. Extract the whole ZIP first."
    }
}
foreach ($file in $files) {
    $source = Join-Path $here $file
    $target = Join-Path $game $file
    if ([IO.Path]::GetFullPath($source) -eq [IO.Path]::GetFullPath($target)) { continue }
    [void][IO.Directory]::CreateDirectory((Split-Path -Parent $target))
    [IO.File]::Copy($source, $target, $true)
}
$loader = Join-Path $game 'winmm.dll'
if (-not (Test-Path -LiteralPath $loader)) {
    $license = Join-Path $game 'EDFModLoader_LICENSE.txt'
    if (-not (Test-Path -LiteralPath $license)) { [IO.File]::Copy((Join-Path $here 'EDFModLoader_LICENSE.txt'), $license) }
    [IO.File]::Copy((Join-Path $here 'winmm.dll'), $loader)
    Write-Host 'Installed the bundled EDFModLoader.'
} else {
    Write-Host 'Kept the existing winmm.dll.'
}
Write-Host 'EDF6Spectate installed. Start EDF6 from Steam as usual.'
Write-Host 'Automatic following only; no switching button yet.'
