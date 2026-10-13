"""Build and test against EDF6_GAME_DIR, then create the install ZIP and SHA-256 file."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import zipfile

from fetch_modloader import main as fetch_loader, TARGET
from fix_winmm_proxy import transform

ROOT = Path(__file__).resolve().parents[1]
FIXED_SHA256 = 'be94e1fac0ca12c41b6924e2eb168851641c999ce951d2a5a9faea5161b0f9a3'


def main():
    version = re.search(r'project\(EDF6Spectate VERSION (\d+\.\d+\.\d+)',
                        (ROOT / 'CMakeLists.txt').read_text()).group(1)
    notes = ROOT / 'release-notes' / f'{version}.md'
    if not notes.is_file():
        raise SystemExit(f'Missing release notes: {notes}')
    if not (Path(os.environ.get('EDF6_GAME_DIR', '')) / 'EDF.dll').is_file():
        raise SystemExit('Set EDF6_GAME_DIR to the installed game. Release game-code tests must not skip.')
    if os.name == 'nt':
        build = ['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                 str(ROOT / 'build.ps1'), '-Test', '-RequireGame']
    else:
        build = ['bash', str(ROOT / 'build.sh'), '-t', '-r']
    subprocess.run(build, cwd=ROOT, check=True)
    subprocess.run([sys.executable, str(ROOT / 'tests/install_test.py')], cwd=ROOT, check=True)
    fetch_loader()
    loader, manifest = transform(TARGET)
    if hashlib.sha256(loader).hexdigest() != FIXED_SHA256:
        raise SystemExit('Fixed EDFModLoader does not match the verified build')

    import json
    payload = {
        'Mods/Plugins/EDF6Spectate.dll': (ROOT / 'out/EDF6Spectate.dll').read_bytes(),
        'winmm.dll': loader,
        'loader-fix.json': (json.dumps(manifest, indent=2) + '\n').encode(),
        'EDFModLoader_LICENSE.txt': (ROOT / 'third_party/EDFModLoader/LICENSE.txt').read_bytes(),
        'EDF6Coop_GUARD_LICENSE.txt': (ROOT / 'third_party/EDF6Coop/LICENSE.txt').read_bytes(),
        'EDF6Coop_TOOLS_LICENSE.txt': (ROOT / 'third_party/EDF6Coop/TOOLS_LICENSE.txt').read_bytes(),
        'RELEASE_NOTES.md': notes.read_bytes(),
    }
    for name in ('INSTALL.bat', 'UNINSTALL.bat', 'install.ps1', 'install.sh', 'README_EDF6Spectate.txt'):
        data = (ROOT / 'packaging' / name).read_bytes()
        if name.endswith('.bat'):
            data = data.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
        payload[name] = data
    release = ROOT / 'release'
    release.mkdir(exist_ok=True)
    archive = release / f'EDF6Spectate-{version}.zip'
    with tempfile.TemporaryDirectory(dir=release) as staging:
        staged = Path(staging) / archive.name
        with zipfile.ZipFile(staged, 'w', compression=zipfile.ZIP_DEFLATED) as zipped:
            for name, data in sorted(payload.items()):
                zipped.writestr(name, data)
        with zipfile.ZipFile(staged) as zipped:
            if zipped.testzip() is not None or set(zipped.namelist()) != set(payload):
                raise SystemExit('Archive verification failed')
            for name, data in payload.items():
                if zipped.read(name) != data:
                    raise SystemExit(f'Archive mismatch: {name}')
        digest = hashlib.sha256(staged.read_bytes()).hexdigest()
        os.replace(staged, archive)
    archive.with_suffix('.zip.sha256').write_text(f'{digest}  {archive.name}\n', encoding='ascii')
    print(f'Package: {archive}\nSHA-256: {digest}')


if __name__ == '__main__':
    main()
