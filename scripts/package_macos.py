#!/usr/bin/env python3
"""Stage a relocatable macOS app from an existing native CMake build."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def run(*args):
    return subprocess.check_output([str(a) for a in args], text=True).strip()


def macho(path):
    if not path.is_file() or path.is_symlink():
        return False
    with path.open('rb') as stream:
        return stream.read(4) in (
            b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe', b'\xfe\xed\xfa\xcf',
            b'\xcf\xfa\xed\xfe', b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca',
            b'\xca\xfe\xba\xbf', b'\xbf\xba\xfe\xca')


def system(path):
    return str(path).startswith(('/System/Library/', '/usr/lib/'))


def dependencies(path):
    # Framework/library paths may contain spaces; split at the compatibility marker.
    return list(dict.fromkeys(re.findall(r'^\s+(.+?) \(compatibility version',
                                         run('otool', '-L', path), re.M)))


def rpaths(path):
    return re.findall(r'cmd LC_RPATH\n\s+cmdsize \d+\n\s+path (.*?) \(offset',
                      run('otool', '-l', path))


class Bundler:
    def __init__(self, app):
        self.app = app.resolve()
        self.frameworks = self.app / 'Contents/Frameworks'
        self.frameworks.mkdir(exist_ok=True)
        self.main = self.app / 'Contents/MacOS/MyPdf'
        self.copied = {}
        self.done = set()
        self.arches = set(run('lipo', '-archs', self.main).split())
        self.main_search = [str(self.expand(p, self.main, self.main)) for p in rpaths(self.main)]

    def expand(self, value, loader, executable):
        return Path(value.replace('@loader_path', str(loader.parent))
                    .replace('@executable_path', str(executable.parent)))

    def resolve(self, value, loader, executable, search):
        if value.startswith('@rpath/'):
            for base in search:
                candidate = self.expand(base, loader, executable) / value[len('@rpath/'):]
                if candidate.exists():
                    return candidate.resolve()
        else:
            candidate = self.expand(value, loader, executable)
            if candidate.is_absolute() and candidate.exists():
                return candidate.resolve()
        raise RuntimeError('Cannot resolve {} required by {}'.format(value, loader))

    def copy(self, source):
        if source.is_relative_to(self.app):
            return source
        if source in self.copied:
            return self.copied[source]
        # Namespace original installations to avoid collisions with macdeployqt's
        # copies or different libraries that happen to share the same basename.
        key = hashlib.sha256(str(source.parent).encode()).hexdigest()[:16]
        library_dir = self.frameworks / 'External' / key
        library_dir.mkdir(parents=True, exist_ok=True)
        framework = next((p for p in source.parents if p.suffix == '.framework'), None)
        if framework:
            root = library_dir / framework.name
            destination = root / source.relative_to(framework)
            if not root.exists():
                shutil.copytree(framework, root, symlinks=True)
        else:
            destination = library_dir / source.name
            shutil.copy2(source, destination)
        self.copied[source] = destination
        return destination

    def visit(self, binary, original=None, executable=None, inherited=()):
        binary = binary.resolve()
        if binary in self.done:
            return
        self.done.add(binary)
        original = original or binary
        executable = executable or self.main
        binary.chmod(binary.stat().st_mode | 0o200)
        arches = set(run('lipo', '-archs', binary).split())
        if not self.arches <= arches:
            raise RuntimeError('Architecture mismatch: {}'.format(binary))
        # Resolve each LC_RPATH in the context where it was defined, then inherit it.
        search = [str(self.expand(p, original, executable)) for p in rpaths(binary)]
        search += list(inherited)
        if executable == self.main:
            search += self.main_search
        if self.signed(binary):
            run('codesign', '--remove-signature', binary)
        identities = re.findall(r'cmd LC_ID_DYLIB\n\s+cmdsize \d+\n\s+name (.*?) \(offset',
                                run('otool', '-l', binary))
        for dependency in dependencies(binary):
            if system(dependency) or dependency in identities:
                continue
            source = self.resolve(dependency, original, executable, search)
            destination = self.copy(source)
            relative = os.path.relpath(destination, binary.parent)
            run('install_name_tool', '-change', dependency, '@loader_path/' + relative, binary)
            self.visit(destination, source, executable, search)
        if identities:
            run('install_name_tool', '-id', '@rpath/' + binary.name, binary)
        # All links now use loader-relative paths; remove build-machine search paths.
        for value in dict.fromkeys(rpaths(binary)):
            run('install_name_tool', '-delete_rpath', value, binary)

    @staticmethod
    def signed(binary):
        return subprocess.run(['codesign', '-d', str(binary)], stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL).returncode == 0

    def verify(self):
        for binary in self.app.rglob('*'):
            if not macho(binary):
                continue
            if rpaths(binary):
                raise RuntimeError('Library search paths remain: {}'.format(binary))
            for value in dependencies(binary):
                if system(value):
                    continue
                # LC_ID_DYLIB is the library's identity, not a dependency.
                if value == '@rpath/' + binary.name:
                    continue
                if not value.startswith('@loader_path/'):
                    raise RuntimeError('External dependency remains: {}: {}'.format(binary, value))
                resolved = self.expand(value, binary, self.main).resolve()
                if not resolved.is_relative_to(self.app) or not resolved.is_file():
                    raise RuntimeError('Invalid bundled dependency: {}'.format(value))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parent.parent
    parser.add_argument('--build-dir', type=Path, default=root / 'build')
    parser.add_argument('--config', default='Debug', help='CMake build configuration (default: Debug)')
    parser.add_argument('--output', type=Path, default=root / 'dist/MyPdf.app')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('Run packaging on macOS using a native macOS build.')
    for tool in ('cmake', 'otool', 'install_name_tool', 'lipo', 'codesign'):
        if not shutil.which(tool):
            parser.error('Required tool not found: ' + tool)
    manifest = args.build_dir / 'generated' / args.config / 'package-tools.txt'
    if not manifest.is_file():
        parser.error('Build metadata missing; run ./scripts/build.sh with the updated CMake files first.')
    source, deploy, qpdf = manifest.read_text().splitlines()
    for path in (source, deploy, qpdf):
        if not path or not Path(path).exists():
            parser.error('Missing app or packaging tool: {!r}; rebuild with Qt macdeployqt and qpdf installed.'.format(path))
    output = args.output.absolute()
    if output.suffix != '.app' or output.exists():
        parser.error('--output must be a new .app path; move or remove the previous package first.')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.mypdf-package-', dir=output.parent) as work:
        app = Path(work) / 'MyPdf.app'
        shutil.copytree(source, app, symlinks=True)
        helpers = app / 'Contents/Helpers'
        helpers.mkdir(exist_ok=True)
        shutil.copy2(qpdf, helpers / 'qpdf')
        # Replace development qt.conf only in the staging copy.
        (app / 'Contents/Resources/qt.conf').unlink(missing_ok=True)
        run(deploy, app, '-always-overwrite', '-no-codesign', '-verbose=1')
        (app / 'Contents/Resources/qt.conf').write_text(
            '[Paths]\nPlugins = PlugIns\nTranslations = Resources/translations\n')
        if not (app / 'Contents/PlugIns/platforms/libqcocoa.dylib').is_file():
            raise RuntimeError('macdeployqt did not deploy the Cocoa platform plugin.')
        bundler = Bundler(app)
        bundler.visit(bundler.main)
        bundler.visit(helpers / 'qpdf', Path(qpdf).resolve(), Path(qpdf).resolve())
        for binary in list(app.rglob('*')):
            if macho(binary):
                bundler.visit(binary)
        bundler.verify()
        for binary in sorted(bundler.done, key=lambda p: len(p.parts), reverse=True):
            run('codesign', '--force', '--sign', '-', binary)
        for framework in sorted((app / 'Contents/Frameworks').rglob('*.framework'),
                                key=lambda p: len(p.parts), reverse=True):
            run('codesign', '--force', '--sign', '-', framework)
        run('codesign', '--force', '--sign', '-', app)
        run('codesign', '--verify', '--deep', '--strict', app)
        environment = os.environ.copy()
        for key in list(environment):
            if key.startswith(('QT_', 'DYLD_')) or key == 'MYPDF_QPDF':
                environment.pop(key)
        environment['PATH'] = '/usr/bin:/bin:/usr/sbin:/sbin'
        environment['MYPDF_SMOKE_TEST'] = '1'
        subprocess.run([str(helpers / 'qpdf'), '--version'], check=True, env=environment, timeout=30)
        subprocess.run([str(bundler.main)], check=True, env=environment, timeout=30)
        app.rename(output)
    print('Packaged app: {}'.format(output))
    print('Local ad-hoc signing applied. Public distribution requires Developer ID signing and notarization.')


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.SubprocessError, ValueError) as error:
        sys.exit('Packaging failed: {}'.format(error))
