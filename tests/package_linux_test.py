"""Linux packaging checks that do not require downloads or FUSE."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('package_linux', ROOT / 'scripts/package_linux.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


class PackagingTest(unittest.TestCase):
    def test_apprun_uses_bundle_and_preserves_relative_document_arguments(self):
        with tempfile.TemporaryDirectory(prefix='MyPdf launcher ') as temp:
            base = Path(temp)
            bundle = base / 'app with spaces'
            binary = bundle / 'usr/bin/MyPdf'
            binary.parent.mkdir(parents=True)
            binary.write_text('''#!/usr/bin/python3
import json, os, sys
print(json.dumps({'cwd': os.getcwd(), 'args': sys.argv[1:], 'qpdf': os.environ['MYPDF_QPDF'],
 'plugins': os.environ['QT_PLUGIN_PATH'], 'libs': os.environ['LD_LIBRARY_PATH'],
 'platform': os.environ.get('QT_QPA_PLATFORM_PLUGIN_PATH')}))
''')
            binary.chmod(0o755)
            apprun = bundle / 'AppRun'
            apprun.write_bytes((ROOT / 'packaging/AppRun').read_bytes())
            apprun.chmod(0o755)
            caller = base / 'caller'
            caller.mkdir()
            env = dict(os.environ, QT_PLUGIN_PATH='/wrong/qt', LD_LIBRARY_PATH='/wrong/libs',
                       QT_QPA_PLATFORM_PLUGIN_PATH='/wrong/platform', MYPDF_QPDF='/wrong/qpdf')
            data = json.loads(subprocess.check_output([str(apprun), 'relative document.pdf', '--other'],
                                                       cwd=caller, env=env, text=True))
            self.assertEqual(data['cwd'], str(caller))
            self.assertEqual(data['args'], ['relative document.pdf', '--other'])
            self.assertEqual(data['qpdf'], str(bundle / 'usr/bin/qpdf'))
            self.assertEqual(data['libs'], str(bundle / 'usr/lib'))
            self.assertEqual(data['plugins'], str(bundle / 'usr/plugins'))
            self.assertIsNone(data['platform'])

    def test_cached_tools_are_verified_without_network_access(self):
        with tempfile.TemporaryDirectory() as temp:
            cache = Path(temp)
            tool = cache / 'tool.AppImage'
            tool.write_bytes(b'trusted tool')
            record = cache / 'tool.AppImage.json'
            record.write_text(json.dumps({'sha256': package.sha256(tool)}))
            with patch.object(package, 'request', side_effect=AssertionError('network used')):
                self.assertEqual(package.fetch_tool(cache, 'owner/repo', tool.name), tool)
                tool.write_bytes(b'corrupt')
                with self.assertRaisesRegex(RuntimeError, 'checksum mismatch'):
                    package.fetch_tool(cache, 'owner/repo', tool.name)

    def test_only_glibc_and_graphics_stack_can_remain_external(self):
        for name in ('libc.so.6', 'libm.so.6', 'libGLX.so.0', 'libX11.so.6', 'libxcb.so.1'):
            self.assertTrue(package.host_library(name), name)
        for name in ('libQt6Core.so.6', 'libqpdf.so.30', 'libpoppler.so.156',
                     'libstdc++.so.6', 'libfontconfig.so.1', 'libglib-2.0.so.0'):
            self.assertFalse(package.host_library(name), name)

    def test_excluded_runtime_libraries_are_explicitly_deployed(self):
        required = {Path('/system/libstdc++.so.6'), Path('/system/libfontconfig.so.1')}
        with patch.object(package, 'dependency_paths', side_effect=[required, set()]), \
             patch.object(package, 'run') as deploy:
            package.bundle_extra_dependencies(Path('/tools'), Path('/app'), {})
            command = deploy.call_args.args
            self.assertIn(Path('/system/libstdc++.so.6'), command)
            self.assertEqual(command.count('--library'), 2)
        with patch.object(package, 'dependency_paths', return_value=required), \
             patch.object(package, 'run'):
            with self.assertRaisesRegex(RuntimeError, 'Cannot bundle remaining'):
                package.bundle_extra_dependencies(Path('/tools'), Path('/app'), {})

    def test_missing_packaged_dependencies_fail_validation(self):
        with tempfile.TemporaryDirectory() as temp:
            appdir = Path(temp)
            (appdir / 'library.so').write_bytes(b'\x7fELFmock')
            result = subprocess.CompletedProcess([], 0, 'libmissing.so => not found\n')
            with patch.object(package.subprocess, 'run', return_value=result):
                with self.assertRaisesRegex(RuntimeError, 'libmissing.so'):
                    package.check_dependencies(appdir)


if __name__ == '__main__':
    unittest.main()
