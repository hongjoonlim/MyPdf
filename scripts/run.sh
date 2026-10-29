#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
case "$(uname -s)" in
    Darwin) exec ./build/MyPdf.app/Contents/MacOS/MyPdf "$@" ;;
    MINGW*|MSYS*|CYGWIN*) exec ./build/MyPdf.exe "$@" ;;
    *) exec ./build/MyPdf "$@" ;;
esac
