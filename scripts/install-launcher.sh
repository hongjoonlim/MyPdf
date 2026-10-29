#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$(uname -s)" != Linux ]]; then
    echo "This desktop launcher installer is for Linux only." >&2
    exit 1
fi
if [[ ! -x build/MyPdf ]]; then
    echo "Build MyPdf first: ./scripts/build.sh" >&2
    exit 1
fi
python3 - "$PWD" "${XDG_DATA_HOME:-$HOME/.local/share}" <<'PY'
from pathlib import Path
import shutil
import subprocess
import sys

project = Path(sys.argv[1]).resolve()
data = Path(sys.argv[2]).expanduser().resolve()
applications = data / 'applications'
icons = data / 'pixmaps'
applications.mkdir(parents=True, exist_ok=True)
icons.mkdir(parents=True, exist_ok=True)
icon = icons / 'MyPdf.png'
shutil.copyfile(project / 'assets/icons/MyPdf.png', icon)

# Desktop entries have their own string and Exec argument escaping rules.
def value(text):
    return str(text).replace('\\', '\\\\').replace('\n', '\\n').replace('\r', '\\r')
def argument(text):
    escaped = str(text)
    for char in ['\\', '"', '`', '$']:
        escaped = escaped.replace(char, '\\' + char)
    return value('"' + escaped + '"').replace('%', '%%')

entry = (project / 'packaging/MyPdf.desktop').read_text()
entry = entry.replace('Exec=MyPdf %f', 'Exec=' + argument(project / 'build/MyPdf') + ' %f')
entry = entry.replace('Icon=MyPdf', 'Icon=' + value(icon))
launcher = applications / 'MyPdf.desktop'
launcher.write_text(entry)
if shutil.which('desktop-file-validate'):
    subprocess.run(['desktop-file-validate', str(launcher)], check=True)
if shutil.which('update-desktop-database'):
    subprocess.run(['update-desktop-database', str(applications)], check=True)
print('Installed launcher: ' + str(launcher))
print('Installed icon: ' + str(icon))
print('Restart MyPdf, then open it from your application menu.')
PY
