"""Stage release notices and application sources; dependency sources remain a release responsibility."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parent.parent


def stage_base(destination):
    destination.mkdir(parents=True, exist_ok=True)
    for name in ('LICENSE', 'NOTICE', 'THIRD_PARTY_NOTICES.md', 'DISTRIBUTING.md'):
        shutil.copy2(ROOT / name, destination / name)
    shutil.copytree(ROOT / 'LICENSES', destination / 'LICENSES', dirs_exist_ok=True)
    # Explicit allowlist prevents build caches, Git credentials/history, local
    # documents and unrelated workspace files from entering a source release.
    entries = ('CMakeLists.txt', 'README.md', 'README.ko.md', 'README.de.md',
               'LICENSE', 'NOTICE', 'THIRD_PARTY_NOTICES.md', 'DISTRIBUTING.md',
               'LICENSES', 'licenses.qrc', 'src', 'translations', 'assets',
               'scripts', 'cmake', 'packaging', 'tests', '.github')
    def source_filter(info):
        if '__pycache__' in Path(info.name).parts or info.name.endswith(('.pyc', '.pyo')):
            return None
        if info.issym() or info.islnk():
            raise RuntimeError('Source snapshot does not accept symlinks: ' + info.name)
        return info
    with tarfile.open(destination / 'MyPdf-source.tar.gz', 'w:gz') as archive:
        for name in entries:
            archive.add(ROOT / name, arcname='MyPdf/' + name, filter=source_filter)


def linux_notices(destination, appdir):
    """Match staged ELF basenames against installed Debian/Ubuntu package records."""
    stage_base(destination)
    records, unresolved, packages = [], [], set()
    names = set()
    for binary in appdir.rglob('*'):
        if binary.is_file() and not binary.is_symlink():
            with binary.open('rb') as stream:
                if stream.read(4) == b'\x7fELF' and binary.name != 'MyPdf':
                    names.add(binary.name)
    for name in sorted(names):
        if not shutil.which('dpkg-query'):
            unresolved.append(name)
            continue
        query = subprocess.run(['dpkg-query', '-S', '/usr/bin/' + name, '/usr/lib/' + name,
                                '/usr/lib/*/' + name, '/lib/' + name, '/lib/*/' + name], text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        owners = set()
        for line in query.stdout.splitlines():
            if ': ' not in line:
                continue
            owner, installed_path = line.rsplit(': ', 1)
            # Ignore unrelated application bundles which happen to carry a
            # private library with the same basename (for example under /opt).
            if (re.fullmatch(r'/(?:usr/)?lib/(?:[^/]+/)?' + re.escape(name), installed_path)
                    or re.fullmatch(r'/(?:usr/)?lib/(?:[^/]+/)?qt6/plugins/[^/]+/' + re.escape(name), installed_path)
                    or installed_path == '/usr/bin/' + name):
                owners.add(owner)
        if not owners:
            unresolved.append(name)
        for owner in sorted(owners):
            packages.add(owner)
    for owner in sorted(packages):
        metadata = subprocess.check_output(['dpkg-query', '-W',
            '-f=${Package}\t${Version}\t${source:Package}\t${source:Version}', owner], text=True).split('\t')
        package, version, source, source_version = metadata
        copyright_file = Path('/usr/share/doc') / package / 'copyright'
        copied = False
        if copyright_file.is_file():
            target = destination / 'dependency-notices' / owner.replace(':', '_')
            target.mkdir(parents=True, exist_ok=True)
            shutil.copy2(copyright_file, target / 'copyright')
            copied = True
        else:
            unresolved.append(owner + ': copyright')
        records.append({'package': package, 'version': version, 'source_package': source,
                        'source_version': source_version, 'notices_copied': copied})
    common = Path('/usr/share/common-licenses')
    if common.is_dir():
        shutil.copytree(common, destination / 'common-licenses', dirs_exist_ok=True)
    write_inventory(destination, records, unresolved)


def homebrew_notices(destination, origins):
    """Copy notices/receipts/formulae from the exact Cellar installations used."""
    stage_base(destination)
    queue, seen, records, unresolved = list(origins), set(), [], []
    while queue:
        origin = Path(queue.pop()).resolve()
        parts = origin.parts
        if 'Cellar' not in parts:
            if not str(origin).startswith(('/System/', '/usr/lib/')):
                unresolved.append(str(origin))
            continue
        index = parts.index('Cellar')
        root = Path(*parts[:index + 3])
        if root in seen:
            continue
        seen.add(root)
        receipt = root / 'INSTALL_RECEIPT.json'
        target = destination / 'dependency-notices' / root.parent.name / root.name
        target.mkdir(parents=True, exist_ok=True)
        matched = []
        # Includes module LICENSES/ and embedded third-party licenses. Avoid
        # traversing library/framework binaries or copying HTML documentation.
        for path in root.rglob('*'):
            relative = path.relative_to(root)
            if not path.is_file() or path.is_symlink():
                continue
            if (re.match(r'^(LICENSE|LICENCE|COPYING|COPYRIGHT|NOTICE)([.\-_]|$)', path.name, re.I)
                    or any(p.lower() in ('licenses', 'licences') for p in relative.parts[:-1])
                    or relative.parts[0] == '.brew'):
                output = target / relative
                output.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, output)
                matched.append(str(relative))
        if receipt.is_file():
            shutil.copy2(receipt, target / receipt.name)
            data = json.loads(receipt.read_text())
            for dep in data.get('runtime_dependencies', []):
                name = dep.get('full_name', '').split('/')[-1]
                version = dep.get('version', '')
                dep_root = root.parents[1] / name / version
                if dep_root.is_dir():
                    queue.append(dep_root)
                else:
                    unresolved.append(name + '@' + version)
        if not matched:
            unresolved.append(str(root) + ': license files')
        records.append({'package': root.parent.name, 'version': root.name, 'notices': matched})
    write_inventory(destination, records, unresolved)


def write_inventory(destination, records, unresolved):
    (destination / 'DEPENDENCIES.json').write_text(json.dumps({
        'packages': records, 'unresolved': sorted(set(unresolved)),
        'dependency_sources_included': False,
        'source_instructions': 'DISTRIBUTING.md'
    }, indent=2) + '\n')
    if unresolved:
        print('License records need release review: ' + ', '.join(sorted(set(unresolved))), flush=True)
