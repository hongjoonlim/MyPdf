"""Exercise dependency relocation without needing Apple's tools on Linux."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('package_macos',
    Path(__file__).resolve().parents[1] / 'scripts/package_macos.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


class PackagingTest(unittest.TestCase):
    def test_qt_paths_prefer_tool_installation_over_shared_prefix(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp).resolve()
            installation = base / 'Cellar/qtbase/6.12'
            shared = base / 'shared'
            for root in (installation, shared):
                (root / 'share/qt/plugins').mkdir(parents=True)
                (root / 'lib').mkdir()
            (installation / 'bin').mkdir()
            qmake = installation / 'bin/qmake'
            qmake.touch(mode=0o755)
            deploy = installation / 'bin/macdeployqt'
            deploy.touch()
            queries = {'QT_INSTALL_PLUGINS': str(shared / 'share/qt/plugins'),
                       'QT_INSTALL_LIBS': str(shared / 'lib'),
                       'QT_INSTALL_PREFIX': str(shared)}
            with patch.object(package, 'run', side_effect=lambda *args: queries[args[-1]]):
                paths = package.qt_paths(deploy)
            self.assertEqual(paths, (installation / 'share/qt/plugins', installation / 'lib'))

    def test_recursive_rpath_and_helper_relocation(self):
        with tempfile.TemporaryDirectory(prefix='MyPdf packaging ') as temp:
            base = Path(temp).resolve()
            app = base / 'MyPdf.app'
            main = app / 'Contents/MacOS/MyPdf'
            helper = app / 'Contents/Helpers/qpdf'
            original_helper = base / 'brew/bin/qpdf'
            poppler = base / 'brew/lib/libpoppler.dylib'
            zlib = base / 'brew/lib/libz.dylib'
            cocoa = app / 'Contents/PlugIns/platforms/libqcocoa.dylib'
            graph = {}
            for path, identity, deps, search in [
                (main, '', ['@rpath/libpoppler.dylib', '/usr/lib/libSystem.B.dylib'], [str(poppler.parent)]),
                (helper, '', ['@executable_path/../lib/libz.dylib'], []),
                (poppler, str(poppler), ['@rpath/libz.dylib'], []),
                (zlib, str(zlib), ['/usr/lib/libSystem.B.dylib'], []),
                (cocoa, str(cocoa), ['@rpath/libz.dylib'], []),
            ]:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'\xcf\xfa\xed\xfe')
                graph[path] = {'id': identity, 'deps': deps, 'rpaths': search}
            original_helper.parent.mkdir(parents=True, exist_ok=True)
            original_helper.write_bytes(helper.read_bytes())
            graph[original_helper] = graph[helper].copy()
            changes = []

            def fake_run(*args):
                tool = args[0]
                if tool == 'lipo':
                    return 'arm64'
                path = Path(args[-1])
                if path not in graph:
                    # Copying preserves the original install ID and load commands.
                    original = next(p for p in (poppler, zlib) if p.name == path.name)
                    graph[path] = {k: list(v) if isinstance(v, list) else v
                                   for k, v in graph[original].items()}
                record = graph[path]
                if tool == 'otool' and args[1] == '-L':
                    names = ([record['id']] if record['id'] else []) + record['deps']
                    return str(path) + ':\n' + '\n'.join(
                        '\t' + name + ' (compatibility version 1.0.0, current version 1.0.0)' for name in names)
                if tool == 'otool':
                    result = ''
                    if record['id']:
                        result += 'cmd LC_ID_DYLIB\n cmdsize 80\n name ' + record['id'] + ' (offset 24)\n'
                    for dep in record['deps']:
                        result += 'cmd LC_LOAD_DYLIB\n cmdsize 80\n name ' + dep + ' (offset 24)\n'
                    for value in record['rpaths']:
                        result += 'cmd LC_RPATH\n cmdsize 80\n path ' + value + ' (offset 12)\n'
                    return result
                if tool == 'install_name_tool':
                    if args[1] == '-change':
                        record['deps'] = [args[3] if d == args[2] else d for d in record['deps']]
                        changes.append((path, args[2], args[3]))
                    elif args[1] == '-id':
                        record['id'] = args[2]
                    else:
                        record['rpaths'].remove(args[2])
                    return ''
                self.fail('Unexpected command: {}'.format(args))

            with patch.object(package, 'run', side_effect=fake_run), \
                 patch.object(package.Bundler, 'signed', return_value=False):
                bundler = package.Bundler(app)
                bundler.visit(main)
                bundler.visit(helper, original_helper, original_helper)
                bundler.visit(cocoa)
                bundler.verify()
                self.assertEqual(len(bundler.copied), 2)
                self.assertEqual(len(changes), 4)
                # Removing an embedded dependency must fail verification.
                bundler.copied[zlib].unlink()
                with self.assertRaisesRegex(RuntimeError, 'Invalid bundled dependency'):
                    bundler.verify()

    def test_widgets_plugin_selection_excludes_unrelated_qml_modules(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp).resolve()
            source = base / 'Qt/plugins'
            app = base / 'MyPdf.app'
            for category, name in [('platforms', 'libqcocoa.dylib'),
                                   ('imageformats', 'libqjpeg.dylib'),
                                   ('styles', 'libqmacstyle.dylib'),
                                   ('platforminputcontexts', 'libqtvirtualkeyboardplugin.dylib'),
                                   ('imageformats', 'libqpdf.dylib'),
                                   ('iconengines', 'libqsvgicon.dylib')]:
                path = source / category / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'plugin')
            stale = app / 'Contents/PlugIns/platforminputcontexts/stale.dylib'
            stale.parent.mkdir(parents=True)
            stale.write_bytes(b'stale')
            origins = package.stage_qt_plugins(app, source)
            self.assertEqual({p.name for p in origins},
                             {'libqcocoa.dylib', 'libqjpeg.dylib', 'libqmacstyle.dylib'})
            self.assertFalse(stale.exists())
            self.assertFalse((app / 'Contents/PlugIns/imageformats/libqpdf.dylib').exists())
            self.assertEqual(origins[app / 'Contents/PlugIns/platforms/libqcocoa.dylib'],
                             source / 'platforms/libqcocoa.dylib')

    def test_matching_qt_framework_fallback_and_deployed_precedence(self):
        with tempfile.TemporaryDirectory(prefix='Qt framework ') as temp:
            base = Path(temp).resolve()
            bundler = package.Bundler.__new__(package.Bundler)
            bundler.frameworks = base / 'MyPdf.app/Contents/Frameworks'
            bundler.library_paths = [str(base / 'Qt/lib')]
            # A copied plugin may have no usable LC_RPATH after macdeployqt.
            suffix = 'QtGui.framework/Versions/A/QtGui'
            source = base / 'Qt/lib' / suffix
            source.parent.mkdir(parents=True)
            source.touch()
            loader = base / 'MyPdf.app/Contents/PlugIns/platforms/libqcocoa.dylib'
            self.assertEqual(bundler.resolve('@rpath/' + suffix, loader, loader, []), source)
            staged = bundler.frameworks / suffix
            staged.parent.mkdir(parents=True)
            staged.touch()
            self.assertEqual(bundler.resolve('@rpath/' + suffix, loader, loader,
                                             [str(source.parents[3])]), staged)
            self.assertEqual(bundler.resolve(str(source), loader, loader, []), staged)
            with self.assertRaisesRegex(RuntimeError, 'Searched:'):
                bundler.resolve('@rpath/Missing.framework/Missing', loader, loader, [])

    def test_otool_parsing_with_spaces_and_arch_headers(self):
        output = 'app (architecture arm64):\n\t/a path/lib.dylib (compatibility version 1.0.0)\n'
        output += 'app (architecture x86_64):\n\t/a path/lib.dylib (compatibility version 1.0.0)\n'
        with patch.object(package, 'run', return_value=output):
            self.assertEqual(package.dependencies(Path('app')), ['/a path/lib.dylib'])


if __name__ == '__main__':
    unittest.main()
