#!/usr/bin/env python3
"""Bundle a native Linux build as an AppImage, without root or FUSE."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parent.parent


def run(*args, **kwargs):
    subprocess.run([str(arg) for arg in args], check=True, **kwargs)


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def request(url):
    return urllib.request.urlopen(urllib.request.Request(url, headers={
        'User-Agent': 'MyPdf-AppImage-packaging', 'Accept': 'application/vnd.github+json'
    }), timeout=120)


def fetch_tool(cache, repo, filename, refresh=False):
    """Cache official release assets with GitHub's published SHA-256 digest."""
    destination = cache / filename
    record = cache / (filename + '.json')
    if not refresh and destination.is_file() and record.is_file():
        data = json.loads(record.read_text())
        if sha256(destination) != data['sha256']:
            raise RuntimeError('Tool checksum mismatch: {}; remove it and retry.'.format(destination))
        return destination
    with request('https://api.github.com/repos/{}/releases/tags/continuous'.format(repo)) as response:
        release = json.load(response)
    asset = next((a for a in release['assets'] if a['name'] == filename), None)
    if not asset or not (asset.get('digest') or '').startswith('sha256:'):
        raise RuntimeError('No checksummed official release asset: {}/{}'.format(repo, filename))
    digest = asset['digest'].split(':', 1)[1]
    url = asset['browser_download_url']
    if not url.startswith('https://github.com/{}/releases/download/'.format(repo)):
        raise RuntimeError('Unexpected release download URL: ' + url)
    print('Downloading {}'.format(filename), flush=True)
    temporary = destination.with_suffix(destination.suffix + '.download')
    try:
        with request(url) as source, temporary.open('wb') as output:
            shutil.copyfileobj(source, output)
        if sha256(temporary) != digest:
            raise RuntimeError('Downloaded tool checksum mismatch: ' + filename)
        temporary.chmod(0o755)
        temporary.replace(destination)
        record.write_text(json.dumps({'repository': repo, 'asset': filename, 'url': url,
            'release': release['published_at'], 'sha256': digest}, indent=2) + '\n')
    finally:
        temporary.unlink(missing_ok=True)
    return destination


def extract_tool(tool, work, name):
    directory = work / name
    directory.mkdir()
    # Extraction is supported by the AppImage runtime without libfuse or mounting.
    run(tool, '--appimage-extract', cwd=directory, stdout=subprocess.DEVNULL)
    return directory / 'squashfs-root'


def clean_environment():
    environment = os.environ.copy()
    for key in list(environment):
        if key.startswith(('QT_', 'QML_', 'LD_')) or key in ('MYPDF_QPDF', 'APPDIR', 'APPIMAGE'):
            environment.pop(key)
    return environment


def host_library(name):
    # glibc/loader and the host display/graphics stack must match the target OS.
    core = ('libc.so.', 'libm.so.', 'libresolv.so.', 'libpthread.so.', 'libdl.so.',
            'librt.so.', 'libutil.so.', 'libanl.so.', 'ld-linux', 'ld64.so.')
    graphics = ('libGL', 'libEGL', 'libOpenGL', 'libGLES', 'libvulkan', 'libdrm',
                'libgbm', 'libX', 'libxcb', 'libwayland', 'libICE.so.', 'libSM.so.')
    return name.startswith(core + graphics)


def dependency_paths(appdir):
    environment = clean_environment()
    environment['LD_LIBRARY_PATH'] = str(appdir / 'usr/lib')
    external = set()
    for binary in appdir.rglob('*'):
        if not binary.is_file() or binary.is_symlink():
            continue
        with binary.open('rb') as stream:
            if stream.read(4) != b'\x7fELF':
                continue
        result = subprocess.run(['ldd', str(binary)], env=environment, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if 'not found' in result.stdout:
            raise RuntimeError('Unresolved packaged dependencies: {}\n{}'.format(binary, result.stdout))
        for path in re.findall(r'=> (/[^\n]+?) \(', result.stdout):
            source = Path(path)
            if not source.is_relative_to(appdir):
                external.add(source)
    return external


def check_dependencies(appdir):
    external = dependency_paths(appdir)
    unexpected = [str(p) for p in external if not host_library(p.name)]
    if unexpected:
        raise RuntimeError('Non-system libraries outside the package: ' + ', '.join(unexpected))
    return sorted({p.name for p in external})


def bundle_extra_dependencies(deploy, appdir, environment):
    # linuxdeploy's generic exclusion list also omits font/C++ libraries. Include
    # those explicitly so only glibc and the host graphics stack remain external.
    previous = set()
    while True:
        needed = {p for p in dependency_paths(appdir) if not host_library(p.name)}
        if not needed:
            return
        if needed == previous:
            raise RuntimeError('Cannot bundle remaining dependencies: ' + str(needed))
        previous = needed
        command = [deploy / 'AppRun', '--appdir', appdir,
                   '--custom-apprun', ROOT / 'packaging/AppRun']
        for library in sorted(needed):
            command.extend(['--library', library])
        run(*command, env=environment)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build')
    parser.add_argument('--config', default='Release')
    parser.add_argument('--output', type=Path, help='New output .AppImage path')
    parser.add_argument('--tools-dir', type=Path, help='Tool cache, also usable offline after initial download')
    parser.add_argument('--refresh-tools', action='store_true', help='Refresh cached official release tools')
    args = parser.parse_args()
    if sys.platform != 'linux':
        parser.error('Run on Linux using a native Linux build.')
    arch = {'amd64': 'x86_64', 'arm64': 'aarch64'}.get(platform.machine(), platform.machine())
    if arch not in ('x86_64', 'aarch64'):
        parser.error('Supported native architectures: x86_64 and aarch64.')
    for tool in ('ldd', 'readelf'):
        if not shutil.which(tool):
            parser.error('Missing build tool: ' + tool)
    build = args.build_dir.resolve()
    cache_text = (build / 'CMakeCache.txt').read_text() if (build / 'CMakeCache.txt').exists() else ''
    configured = next((line.split('=', 1)[1] for line in cache_text.splitlines()
                       if line.startswith('CMAKE_BUILD_TYPE:STRING=')), '')
    if configured and configured != args.config:
        parser.error('Configured build is {}; use --config {} or rebuild the requested configuration.'.format(configured, configured))
    metadata = build / 'generated' / args.config / 'linux-package-tools.txt'
    if not metadata.is_file():
        parser.error('Build metadata missing. Configure/build Release using the current CMakeLists.txt first.')
    executable, qmake, qpdf, version = metadata.read_text().splitlines()
    for value in (executable, qmake, qpdf):
        if not value or not os.access(value, os.X_OK):
            parser.error('Missing executable: {!r}. Reconfigure with Qt 6 and qpdf installed.'.format(value))
    machine = {'x86_64': 'Advanced Micro Devices X86-64', 'aarch64': 'AArch64'}[arch]
    for binary in (executable, qpdf):
        header = subprocess.check_output(['readelf', '--file-header', binary], text=True,
                                         env=dict(os.environ, LC_ALL='C'))
        if machine not in header:
            parser.error('Executable architecture does not match {}: {}'.format(arch, binary))
    output = (args.output or ROOT / 'dist' / 'MyPdf-{}-{}.AppImage'.format(version, arch)).absolute()
    checksum = output.with_name(output.name + '.sha256')
    manifest = output.with_name(output.name + '.build.json')
    if output.suffix != '.AppImage' or any(p.exists() for p in (output, checksum, manifest)):
        parser.error('Choose a new .AppImage path; existing packages are never overwritten.')
    cache = (args.tools_dir or build / 'appimage-tools').resolve()
    cache.mkdir(parents=True, exist_ok=True)
    tools = [fetch_tool(cache, 'linuxdeploy/linuxdeploy', 'linuxdeploy-{}.AppImage'.format(arch), args.refresh_tools),
             fetch_tool(cache, 'linuxdeploy/linuxdeploy-plugin-qt', 'linuxdeploy-plugin-qt-{}.AppImage'.format(arch), args.refresh_tools),
             fetch_tool(cache, 'AppImage/type2-runtime', 'runtime-{}'.format(arch), args.refresh_tools)]
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.mypdf-appimage-', dir=output.parent) as temporary:
        work = Path(temporary)
        deploy = extract_tool(tools[0], work, 'deploy')
        qt = extract_tool(tools[1], work, 'qt')
        # linuxdeploy discovers executable plugins next to its binary or on PATH.
        plugin = work / 'linuxdeploy-plugin-qt'
        plugin.write_text('#!/bin/sh\nexec {} "$@"\n'.format(shlex.quote(str(qt / 'AppRun'))))
        plugin.chmod(0o755)
        appdir = work / 'MyPdf.AppDir'
        environment = clean_environment()
        environment.update({'PATH': str(work) + os.pathsep + os.environ['PATH'],
            'QMAKE': qmake, 'NO_STRIP': '1', 'EXTRA_PLATFORM_PLUGINS': 'libqoffscreen.so',
            'LDAI_RUNTIME_FILE': str(tools[2]), 'LDAI_OUTPUT': str(work / output.name),
            'LDAI_NO_APPSTREAM': '1', 'ARCH': arch, 'VERSION': version})
        # XCB works on X11 and Wayland desktops with XWayland, without requiring
        # Qt Wayland modules on the build machine.
        run(deploy / 'AppRun', '--appdir', appdir, '--executable', executable,
            '--executable', qpdf, '--desktop-file', ROOT / 'packaging/MyPdf.desktop',
            '--icon-file', ROOT / 'assets/icons/256x256/MyPdf.png', '--plugin', 'qt',
            '--custom-apprun', ROOT / 'packaging/AppRun', env=environment)
        bundle_extra_dependencies(deploy, appdir, environment)
        system_libraries = check_dependencies(appdir)
        run(deploy / 'AppRun', '--appdir', appdir, '--custom-apprun', ROOT / 'packaging/AppRun',
            '--output', 'appimage', env=environment)
        image = work / output.name
        # Exercise the actual final image without FUSE, from a path containing
        # spaces and without build-machine Qt overrides or qpdf on PATH.
        relocated = work / 'relocated app'
        relocated.mkdir()
        run(image, '--appimage-extract', cwd=relocated, stdout=subprocess.DEVNULL)
        extracted = relocated / 'squashfs-root'
        testenv = clean_environment()
        testenv.update({'PATH': '/usr/bin:/bin', 'QT_QPA_PLATFORM': 'offscreen', 'MYPDF_SMOKE_TEST': '1',
                        'LD_LIBRARY_PATH': str(extracted / 'usr/lib'),
                        'XDG_CONFIG_HOME': str(work / 'config'), 'XDG_CACHE_HOME': str(work / 'cache')})
        run(extracted / 'usr/bin/qpdf', '--version', env=testenv, timeout=30)
        run(extracted / 'AppRun', env=testenv, timeout=30)
        if (build / 'pdfcore_test').is_file():
            testenv.update({'QT_PLUGIN_PATH': str(extracted / 'usr/plugins'),
                            'MYPDF_QPDF': str(extracted / 'usr/bin/qpdf')})
            run(build / 'pdfcore_test', env=testenv, timeout=120)
        image.chmod(0o755)
        image.rename(output)
    checksum.write_text('{}  {}\n'.format(sha256(output), output.name))
    manifest.write_text(json.dumps({'version': version, 'architecture': arch,
        'build_os': platform.platform(), 'libc': platform.libc_ver(),
        'system_libraries': system_libraries,
        'tools': [json.loads((cache / (t.name + '.json')).read_text()) for t in tools]}, indent=2) + '\n')
    print('Packaged: {}\nChecksum: {}'.format(output, checksum))


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        sys.exit('Packaging failed: {}'.format(error))
