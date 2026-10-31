#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
args=()
if [[ "$(uname -s)" == Darwin ]] && command -v brew >/dev/null; then
    args+=("-DCMAKE_PREFIX_PATH=$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)")
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
