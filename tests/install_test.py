"""Exercise the shipped installers in temporary game folders, without a game binary.

Defects: updates can overwrite a shared loader; uninstall can delete another mod;
an incomplete download can mutate the installation before reporting missing files.
Run the real scripts and assert the files users would have afterwards.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = 'Mods/Plugins/EDF6Spectate.dll'


class InstallTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='spectate-install-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.package = self.root / 'package [1] with spaces'
        self.package.mkdir()
        for name in ('install.sh', 'install.ps1'):
            shutil.copyfile(ROOT / 'packaging' / name, self.package / name)
        for name in (PLUGIN, 'winmm.dll', 'EDFModLoader_LICENSE.txt', 'README_EDF6Spectate.txt',
                     'EDF6Coop_GUARD_LICENSE.txt', 'EDF6Coop_TOOLS_LICENSE.txt'):
            path = self.package / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(('packaged ' + name).encode())

    def run_installer(self, shell, game, uninstall=False, success=True):
        if shell == 'bash':
            command = ['bash', str(self.package / 'install.sh')]
            if uninstall:
                command.append('--uninstall')
            command.append(str(game))
        else:
            command = [shell, '-NoProfile', '-File', str(self.package / 'install.ps1'), '-GameDir', str(game)]
            if uninstall:
                command.append('-Uninstall')
        result = subprocess.run(command, capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode == 0, success, result.stdout + result.stderr)

    def shells(self):
        if os.name != 'nt':
            yield 'bash'
        shell = shutil.which('powershell' if os.name == 'nt' else 'pwsh')
        if shell:
            yield shell
        else:
            self.fail('PowerShell is required to verify the Windows installer')

    def test_install_update_uninstall_preserve_shared_files(self):
        for index, shell in enumerate(self.shells()):
            for existing in (False, True):
                with self.subTest(shell=shell, existing_loader=existing):
                    game = self.root / f'game [{index}] {existing}'
                    (game / 'Mods/Plugins').mkdir(parents=True)
                    (game / 'EDF6.exe').touch()
                    protected = {'Mods/Plugins/Other.dll': b'other mod', 'ModLoader.ini': b'user settings',
                                 'Mods/Plugins/EDF6Spectate.log': b'previous log'}
                    if existing:
                        protected['winmm.dll'] = b'custom loader'
                        protected['EDFModLoader_LICENSE.txt'] = b'custom license'
                    for name, data in protected.items():
                        (game / name).write_bytes(data)
                    self.run_installer(shell, game)
                    self.assertEqual((game / PLUGIN).read_bytes(), (self.package / PLUGIN).read_bytes())
                    if not existing:
                        self.assertEqual((game / 'winmm.dll').read_bytes(), (self.package / 'winmm.dll').read_bytes())
                    (game / PLUGIN).write_bytes(b'old version')
                    self.run_installer(shell, game)
                    self.assertEqual((game / PLUGIN).read_bytes(), (self.package / PLUGIN).read_bytes())
                    self.run_installer(shell, game, uninstall=True)
                    self.run_installer(shell, game, uninstall=True)
                    self.assertFalse((game / PLUGIN).exists())
                    self.assertTrue((game / 'winmm.dll').is_file())
                    for name, data in protected.items():
                        self.assertEqual((game / name).read_bytes(), data)

    def test_incomplete_package_and_invalid_game_leave_files_unchanged(self):
        (self.package / 'winmm.dll').unlink()
        for index, shell in enumerate(self.shells()):
            game = self.root / f'game {index}'
            game.mkdir()
            self.run_installer(shell, game, success=False)
            self.assertEqual(list(game.iterdir()), [])
            (game / 'EDF6.exe').touch()
            self.run_installer(shell, game, success=False)
            self.assertEqual([p.name for p in game.iterdir()], ['EDF6.exe'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
