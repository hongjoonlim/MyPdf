#!/usr/bin/env bash
set -euo pipefail
script_dir="$(dirname "${BASH_SOURCE[0]}")"
exec python3 "$script_dir/package_linux.py" "$@"
