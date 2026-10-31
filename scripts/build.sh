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
    # qpdf is a runtime tool as well as a test dependency. Do not rely on shell PATH.
    qpdf_prefix="$(brew --prefix qpdf 2>/dev/null || true)"
    qpdf_executable="${MYPDF_QPDF:-}"
    if [[ -z "$qpdf_executable" && -n "$qpdf_prefix" && -x "$qpdf_prefix/bin/qpdf" ]]; then
        qpdf_executable="$qpdf_prefix/bin/qpdf"
    fi
    if [[ -z "$qpdf_executable" ]]; then
        qpdf_executable="$(command -v qpdf || true)"
    fi
    if [[ -z "$qpdf_executable" || ! -f "$qpdf_executable" || ! -x "$qpdf_executable" ]]; then
        printf 'qpdf executable was not found. Install it with: brew install qpdf\n' >&2
        printf 'If already installed, check "$(brew --prefix qpdf)/bin/qpdf" --version.\n' >&2
        printf 'If MYPDF_QPDF is set, ensure it names an executable file.\n' >&2
        exit 1
    fi
    args+=("-DQPDF_EXECUTABLE=$qpdf_executable")
    printf 'qpdf: %s\n' "$qpdf_executable"
    # Qt probes these headers optionally; no Vulkan runtime is needed by MyPdf.
    vulkan_prefix="$(brew --prefix vulkan-headers 2>/dev/null || true)"
    if [[ -n "$vulkan_prefix" && -f "$vulkan_prefix/include/vulkan/vulkan.h" ]]; then
        args+=("-DVulkan_INCLUDE_DIR=$vulkan_prefix/include")
    fi
fi
if [[ "${MSYSTEM:-}" == UCRT64 ]]; then
    args+=(-G Ninja "-DCMAKE_PREFIX_PATH=$MINGW_PREFIX")
fi
# Allow an explicit test-platform choice without editing CTest's generated files.
if [[ -n "${MYPDF_TEST_QPA_PLATFORM:-}" ]]; then
    args+=("-DMYPDF_TEST_QPA_PLATFORM=$MYPDF_TEST_QPA_PLATFORM")
fi
if [[ -n "${MYPDF_TEST_QT_PLUGIN_DIR:-}" ]]; then
    args+=("-DMYPDF_TEST_QT_PLUGIN_DIR=$MYPDF_TEST_QT_PLUGIN_DIR")
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug "${args[@]}"
cmake --build build --parallel
if ! ctest --test-dir build --output-on-failure; then
    printf '\nTests failed. The build completed, but testing did not pass.\n' >&2
    printf 'Full test log: build/Testing/Temporary/LastTest.log\n' >&2
    printf 'For GUI startup failures, run:\n' >&2
    printf "  QT_DEBUG_PLUGINS=1 ctest --test-dir build -R '^pdf_operations$' -V\n" >&2
    exit 1
fi
