#!/usr/bin/env bash
set -euo pipefail

# Resolve the build relative to this script without changing the caller's directory.
# Relative document arguments therefore retain their original meaning.
script_dir="$(dirname "${BASH_SOURCE[0]}")"
build_dir="${MYPDF_BUILD_DIR:-$script_dir/../build}"
candidates=()
case "$(uname -s)" in
    Darwin)
        candidates+=("$build_dir/MyPdf.app/Contents/MacOS/MyPdf")
        for configuration in Debug Release RelWithDebInfo MinSizeRel; do
            candidates+=("$build_dir/$configuration/MyPdf.app/Contents/MacOS/MyPdf")
        done
        ;;
    MINGW*|MSYS*|CYGWIN*)
        candidates+=("$build_dir/MyPdf.exe")
        for configuration in Debug Release RelWithDebInfo MinSizeRel; do
            candidates+=("$build_dir/$configuration/MyPdf.exe")
        done
        ;;
    *) candidates+=("$build_dir/MyPdf") ;;
esac

for executable in "${candidates[@]}"; do
    if [[ -f "$executable" && -x "$executable" ]]; then
        exec "$executable" "$@"
    fi
done

printf 'MyPdf executable not found. Checked:\n' >&2
printf '  %s\n' "${candidates[@]}" >&2
printf 'Run ./scripts/build.sh from the project directory first.\n' >&2
printf 'For a different build folder, set MYPDF_BUILD_DIR when running this script.\n' >&2
exit 1
