"""Verify release notice/source staging independently of a native package build."""
import json
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import package_notices as notices


class NoticesTest(unittest.TestCase):
    def test_application_source_snapshot_and_license_texts(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / 'licenses'
            notices.stage_base(output)
            self.assertIn('GNU GENERAL PUBLIC LICENSE', (output / 'LICENSE').read_text())
            self.assertTrue((output / 'LICENSES/AppImage-runtime.txt').is_file())
            with tarfile.open(output / 'MyPdf-source.tar.gz') as archive:
                names = archive.getnames()
                self.assertIn('MyPdf/src/forms.cpp', names)
                self.assertIn('MyPdf/scripts/package_notices.py', names)
                self.assertIn('MyPdf/licenses.qrc', names)
                self.assertFalse(any('/build/' in p or '/.git/' in p or '__pycache__' in p for p in names))

    def test_homebrew_dependency_notices_and_receipts(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            root = base / 'Cellar/example/1.0'
            root.mkdir(parents=True)
            (root / 'LICENSE').write_text('Example library license')
            (root / 'INSTALL_RECEIPT.json').write_text(json.dumps({'runtime_dependencies': []}))
            output = base / 'licenses'
            with patch.object(notices, 'stage_base', side_effect=lambda p: p.mkdir()):
                notices.homebrew_notices(output, [root / 'lib/example.dylib', root])
            data = json.loads((output / 'DEPENDENCIES.json').read_text())
            self.assertEqual(len(data['packages']), 1)
            self.assertEqual(data['unresolved'], [])
            self.assertFalse(data['dependency_sources_included'])
            self.assertEqual((output / 'dependency-notices/example/1.0/LICENSE').read_text(), 'Example library license')

    def test_linux_ignores_private_copies_in_unrelated_apps(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            appdir = base / 'app'; appdir.mkdir()
            (appdir / 'libz.so.1').write_bytes(b'\x7fELF')
            output = base / 'licenses'
            result = SimpleNamespace(stdout='zlib1g: /usr/lib/x86_64-linux-gnu/libz.so.1\nunrelated: /opt/private/libz.so.1\n')
            with patch.object(notices, 'stage_base', side_effect=lambda p: p.mkdir()), patch.object(notices.shutil, 'which', return_value='/usr/bin/dpkg-query'), patch.object(notices.subprocess, 'run', return_value=result), patch.object(notices.subprocess, 'check_output', return_value='zlib1g\t1.0\tzlib\t1.0') as metadata:
                notices.linux_notices(output, appdir)
            self.assertEqual(metadata.call_count, 1)
            self.assertEqual(metadata.call_args.args[0][-1], 'zlib1g')
            data = json.loads((output / 'DEPENDENCIES.json').read_text())
            self.assertEqual([p['package'] for p in data['packages']], ['zlib1g'])

    def test_unknown_linux_binary_remains_explicitly_unresolved(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            appdir = base / 'app'; appdir.mkdir()
            (appdir / 'custom.so').write_bytes(b'\x7fELF')
            output = base / 'licenses'
            with patch.object(notices, 'stage_base', side_effect=lambda p: p.mkdir()), patch.object(notices.shutil, 'which', return_value=None):
                notices.linux_notices(output, appdir)
            data = json.loads((output / 'DEPENDENCIES.json').read_text())
            self.assertIn('custom.so', data['unresolved'])


if __name__ == '__main__':
    unittest.main()
