#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
args=()
if [[ "$(uname -s)" == Darwin ]] && command -v brew >/dev/null; then
    poppler_prefix="$(brew --prefix poppler-qt6 2>/dev/null || true)"
    if [[ -z "$poppler_prefix" || ! -f "$poppler_prefix/include/poppler/qt6/poppler-qt6.h" || ! -f "$poppler_prefix/lib/libpoppler-qt6.dylib" ]]; then
        printf 'Poppler Qt 6 development files were not found in the active Homebrew installation.\n' >&2
        printf 'Install them with: brew install poppler-qt6 pkgconf\n' >&2
        printf 'If already installed, check brew list --versions poppler-qt6 and try brew reinstall poppler-qt6.\n' >&2
        exit 1
    fi
    args+=("-DCMAKE_PREFIX_PATH=$(brew --prefix qtbase);$poppler_prefix;$(brew --prefix zlib)")
    args+=("-DPOPPLER_INCLUDE_DIR=$poppler_prefix/include/poppler/qt6" "-DPOPPLER_LIBRARY=$poppler_prefix/lib/libpoppler-qt6.dylib")
    export PKG_CONFIG_PATH="$poppler_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    printf 'Poppler Qt 6: %s\n' "$poppler_prefix"
    # Qt probes these headers optionally; no Vulkan runtime is needed by MyPdf.
    vulkan_prefix="$(brew --prefix vulkan-headers 2>/dev/null || true)"
    if [[ -n "$vulkan_prefix" && -f "$vulkan_prefix/include/vulkan/vulkan.h" ]]; then
        args+=("-DVulkan_INCLUDE_DIR=$vulkan_prefix/include")
    fi
fi
if [[ "${MSYSTEM:-}" == UCRT64 ]]; then
    args+=(-G Ninja "-DCMAKE_PREFIX_PATH=$MINGW_PREFIX")
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug "${args[@]}"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
