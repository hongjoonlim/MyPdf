# MyPdf

**English** | [Deutsch](README.de.md) | [한국어](README.ko.md)

A C++17 / Qt 6 PDF editor targeting Ubuntu, Windows, and macOS. Read documents, organize pages, and add handwritten or image signatures. PDF rendering uses Poppler; page operations use qpdf.

## Features

- Open and close PDF files.
- Import DOCX documents as editable PDFs; legacy DOC is not supported.
- Draw a signature or import a signature image.
- Split documents by extracting selected pages into separate PDFs.
- Move pages earlier or later in the document.
- Save changes or save a copy under a different name.
- Insert blank pages, all pages from another PDF, or all pages generated from a DOCX.
- Switch the interface between English, German, and Korean without restarting.
- Switch between light and dark appearance; the choice is saved.

Signatures are **visual signatures**, not certificate-based digital signatures.

The application icon is the selected Pomeranian concept (08), stored in `assets/icons/MyPdf.png` and embedded in the executable. Linux desktop installation also installs `packaging/MyPdf.desktop` and the icon. From the project directory, `cmake --install build --prefix "$HOME/.local"` installs the application for the current user; the installed desktop entry uses absolute executable and icon paths, so launching from the application menu does not depend on `PATH`. The icon is also installed in the hicolor theme and available desktop/icon caches are refreshed.

For Linux development, run `./scripts/install-launcher.sh` after building to register the current build and icon in your user application menu without copying the executable. Restart MyPdf and launch it from the application menu. If an old dock favorite remains, remove and pin it again. Run the script again after moving the project. This registers the launcher under `${XDG_DATA_HOME:-$HOME/.local/share}`.

## Build and run

Requires CMake 3.22+, a C++17 compiler, Qt 6.4+ Widgets, Poppler's Qt 6 bindings, qpdf, and zlib. Build natively on each target OS with matching architecture and compiler dependencies. Do not reuse one OS's `build/` directory on another OS or change its CMake generator in place.

| Platform | Configuration | Validation status |
| --- | --- | --- |
| Ubuntu 26.04 | GCC, system Qt / Poppler / qpdf | Built and tested locally after these changes. |
| Windows x64 | MSYS2 UCRT64, GCC, Ninja | Build configuration and native CI added; not executed on Windows in this session. |
| macOS | Apple Clang, Homebrew, native architecture | Build configuration and native CI added; not executed on macOS in this session. |

### Ubuntu

Run from the project directory:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config qt6-base-dev libpoppler-qt6-dev qpdf qt6-translations-l10n zlib1g-dev
./scripts/build.sh
./scripts/run.sh
```

### Windows: MSYS2 UCRT64

Install [MSYS2](https://www.msys2.org/), open its **UCRT64** terminal, and change to the project directory, for example `cd /c/dev/MyPdf`. Run the following commands. If the first update asks you to close the terminal, reopen UCRT64 and finish updating before installing packages.

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-translations mingw-w64-ucrt-x86_64-poppler-qt6 mingw-w64-ucrt-x86_64-qpdf mingw-w64-ucrt-x86_64-zlib
./scripts/build.sh
./scripts/run.sh
```

Output: `build/MyPdf.exe`, including the Pomeranian `.ico` resource. Launch from UCRT64 so its Qt, Poppler, and compiler DLLs remain on `PATH`. This is the documented MinGW route; do not mix these libraries with an MSVC Qt installation. Package reference: [Poppler Qt 6 for UCRT64](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-poppler-qt6).

### macOS: Homebrew

Install [Homebrew](https://brew.sh/) first. In the project directory, run the following; skip `xcode-select --install` if Command Line Tools are already installed.

```bash
xcode-select --install
brew install cmake ninja pkgconf qtbase qttranslations poppler-qt6 qpdf zlib vulkan-headers
./scripts/build.sh
./scripts/run.sh
```

`build.sh` supplies the Homebrew Qt and Poppler prefixes automatically. Use **poppler-qt6**, not just `poppler`: its Qt 6 bindings are required and the formula is keg-only. See [the Homebrew formula](https://formulae.brew.sh/formula/poppler-qt6). Output: `build/MyPdf.app`; the executable is `build/MyPdf.app/Contents/MacOS/MyPdf`. The bundle includes the Pomeranian `.icns` icon. Use native dependencies for Apple Silicon or Intel; this does not create a universal binary.

#### macOS: `Could not find POPPLER_INCLUDE_DIR ... poppler-qt6.h`

This error means the **required Poppler Qt 6 development headers** were not found, unlike the optional Vulkan probe above. Plain `poppler` is not sufficient. Homebrew's separate [poppler-qt6 package](https://formulae.brew.sh/formula/poppler-qt6) is keg-only, so its files may be outside the default search path even when installed.

```bash
brew install poppler-qt6 pkgconf
brew list --versions poppler-qt6
ls "$(brew --prefix poppler-qt6)/include/poppler/qt6/poppler-qt6.h"
./scripts/build.sh
```

The updated `build.sh` checks that the header and library exist and explicitly supplies Homebrew's pkg-config path, `POPPLER_INCLUDE_DIR` and `POPPLER_LIBRARY`. Update the script in your Mac checkout as well. If Homebrew lists the package but the header is absent, use `brew reinstall poppler-qt6`. Keep Qt, Poppler and the compiler on the same architecture; do not mix Apple Silicon and Intel/Rosetta Homebrew installations.

For manual CMake configuration:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)" \
  -DPOPPLER_INCLUDE_DIR="$(brew --prefix poppler-qt6)/include/poppler/qt6" \
  -DPOPPLER_LIBRARY="$(brew --prefix poppler-qt6)/lib/libpoppler-qt6.dylib"
cmake --build build --parallel
```

#### macOS: `Tests require qpdf on PATH or -DQPDF_EXECUTABLE=...`

CMake could not locate the **qpdf executable**. It is used by tests and by page editing, signatures and text composition at runtime; disabling tests does not solve the runtime dependency. Install [Homebrew's qpdf package](https://formulae.brew.sh/formula/qpdf) and check the executable:

```bash
brew install qpdf
"$(brew --prefix qpdf)/bin/qpdf" --version
./scripts/build.sh
```

The updated `build.sh` checks `MYPDF_QPDF`, Homebrew's `qpdf/bin/qpdf`, then `PATH`, and passes `-DQPDF_EXECUTABLE` to CMake. Update the script in your Mac checkout as well. If installed but the version command fails, use `brew reinstall qpdf`. If `MYPDF_QPDF` is set, ensure it names a valid executable or unset it. For manual CMake configuration, add `-DQPDF_EXECUTABLE="$(brew --prefix qpdf)/bin/qpdf"` to the existing configure command.

#### macOS: several GUI tests exit with `Subprocess aborted`

`native_docx`, `pdf_operations`, `localization`, `signature_cancellation`, `editable_objects` and `object_editing` create a `QApplication`. If only `tool_discovery` passes, a shared GUI initialization failure such as Qt platform-plugin loading is plausible, but the failure list alone does not establish the cause. A successful build is separate from passing tests.

The old configuration forced `offscreen` for all GUI tests, which could abort on macOS with `Could not find the Qt platform plugin "offscreen"`. The updated defaults are `cocoa` on macOS and `offscreen` elsewhere. When a matching Qt plugin CMake target is available, its actual directory is supplied as `QT_QPA_PLATFORM_PLUGIN_PATH` and `QT_PLUGIN_PATH` for tests only; global shell settings remain unchanged.

Update `CMakeLists.txt`, `cmake/test_environment.cmake` and `scripts/build.sh` in the Mac checkout, then run the following. Cocoa tests need a logged-in macOS desktop session and may briefly show windows or dialogs.

```bash
MYPDF_TEST_QPA_PLATFORM=cocoa ./scripts/build.sh
```

For headless tests with a Qt installation that includes the offscreen plugin, use `MYPDF_TEST_QPA_PLATFORM=offscreen` instead. To supply a plugin directory manually, set `MYPDF_TEST_QT_PLUGIN_DIR` to the matching Qt version/architecture's `platforms` directory. The script forwards these as CMake options, so selections persist in the build cache.

If startup still aborts, obtain plugin diagnostics:

```bash
QT_DEBUG_PLUGINS=1 ctest --test-dir build -R '^pdf_operations$' -V
```

The full test log is `build/Testing/Temporary/LastTest.log`. Diagnose from the actual output, such as `Could not find/load the Qt platform plugin`, `Library not loaded` or `incompatible architecture`. Check whether plugin paths point to another Qt/Python installation and whether Qt and its plugins have matching versions and architectures. `QT_DEBUG_PLUGINS` is an [official Qt plugin diagnostic option](https://doc.qt.io/qt-6/deployment-plugins.html). Test-specific CTest environment settings override the parent environment: setting only `QT_QPA_PLATFORM=cocoa` in the shell does not override this configuration. Disabling tests or installing more packages without checking the log does not establish a fix.

#### macOS: `Could NOT find WrapVulkanHeaders (missing Vulkan_INCLUDE_DIR)`

This is Qt's probe for optional Vulkan headers, not necessarily a fatal build error. MyPdf uses Qt Widgets, QPainter and Poppler, and does not call Vulkan. If CMake reaches `Configuring done` and `Generating done`, this message did not stop configuration. If configuration fails, inspect the following `CMake Error` and dependency messages as well. Qt marks this dependency optional in its [configuration source](https://github.com/qt/qtbase/blob/6.10/src/gui/configure.cmake#L74-L75).

The macOS setup command above includes [Homebrew's `vulkan-headers`](https://formulae.brew.sh/formula/vulkan-headers) to satisfy the probe. If you followed an older README, install it and retry:

```bash
brew install vulkan-headers
./scripts/build.sh
```

`build.sh` detects the installed Homebrew headers and passes `Vulkan_INCLUDE_DIR` automatically. For manual configuration, use:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)" \
  -DVulkan_INCLUDE_DIR="$(brew --prefix vulkan-headers)/include"
cmake --build build --parallel
```

For this message, a Vulkan SDK, Vulkan loader or MoltenVK runtime is not required by MyPdf; the header package is sufficient. Installing headers does not resolve unrelated Qt/Poppler dependency errors.


### Shared commands and installation

`build.sh` creates a Debug build and runs the tests. `run.sh` selects the platform-specific executable. For a Release build, configure the same dependency prefixes first; Windows needs `-G Ninja -DCMAKE_PREFIX_PATH="$MINGW_PREFIX"`, and macOS needs `-DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)"` on the configure command below.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./scripts/run.sh '/path/to/document.pdf'
```

`run.sh` locates the build relative to the script, without changing the working directory: relative document paths are resolved from the directory where you launch it. macOS and Windows also support configuration subdirectories such as `build/Debug` and `build/Release`. For a custom build directory, set `MYPDF_BUILD_DIR` (relative to your current directory, or absolute). Build directories and CMake caches are machine-specific; configure a fresh build on each operating system.

For a development installation, use `cmake --install build --prefix /path/to/install`. macOS installs `MyPdf.app`; Windows installs `bin/MyPdf.exe`; Linux also installs the desktop entry and icon. These installs are **not self-contained distribution packages**: Qt plugins/libraries, Poppler dependencies, qpdf and its dependencies must still be available. Automatic DLL/dylib deployment, installers, macOS signing and notarization are not implemented.

qpdf lookup order: `MYPDF_QPDF`, an executable beside the application, `../Helpers/qpdf` (or `qpdf.exe`), `PATH`, then the executable found at configure time. A configure-time path is a development fallback tied to the build machine. For Finder launches, ensure qpdf is available at a supported location or use `run.sh` from a configured shell. Set `MYPDF_QPDF` to an executable path, without extra command arguments, to override discovery.

The workflow [`.github/workflows/build.yml`](.github/workflows/build.yml) builds and tests on Ubuntu, Windows UCRT64, and macOS when pushed to GitHub. Adding the workflow does not mean those remote jobs have already passed.

## Workspace

- **Menu bar:** File, Edit, View, Pages, Settings, and Help. Text and signature commands live together in Edit; page operations are directly available in Pages.
- **Main toolbar:** Open and Save, the filename, then Select, Text, Sign and Pages. Sign draws a signature directly; its arrow opens image import. Pages opens a compact menu for moving, inserting and extracting. Save As and Close Document stay in File.
- **Context actions:** Edit and Delete appear beside the main tools only while an added object is selected. Right-click that object for the same actions, or double-click to edit. A Cancel button appears only while placing text or a signature. There are no task tabs.
- **Left sidebar:** Page thumbnails with multiple selection and a Pages context menu. Toggle it through View → Page Thumbnails.
- **Bottom navigation:** Previous/next page, page number input, zoom out/in, zoom input from 40% to 300%, and a fit menu (page, width, actual size). Documents initially open fitted to the page.

The application uses consistent monochrome outline icons, including file dialogs and message boxes.

## Interface language

Choose **Settings → Language → English / Deutsch / 한국어**. The equivalent paths are **Einstellungen → Sprache** in German and **설정 → 언어** in Korean.

Changes apply immediately to menus, toolbars, page labels, and application messages. The document, unsaved edits, page selection, zoom, and active tool are preserved.

English is the default and fallback language. Your selection is saved as `ui/language` through Qt `QSettings`; an existing preference takes priority. On Ubuntu, the default file is `~/.config/MyPdf/MyPdf.conf`, subject to `XDG_CONFIG_HOME`.

Qt dialog translations require the platform-specific translations package listed above (`qt6-translations-l10n` on Ubuntu). Without it, some standard dialog text may remain in English. File pickers use Qt dialogs so they follow the selected application language.

## Appearance

Choose **Settings → Appearance → Light / Dark** to change the interface immediately. The selected theme is saved as `ui/theme` through Qt `QSettings`; light is the default. PDF pages retain their document colors.

## Usage

Shortcut notation below follows Ubuntu/Windows. Standard Qt shortcuts use Command on macOS where appropriate.

| Task | How to use it |
| --- | --- |
| Open / close | File menu or `Ctrl+O` / `Ctrl+W`. Unsaved changes prompt you to save. |
| Add a signature | Choose **Draw Signature…** or **Import Signature Image…**, then drag a rectangle on the page to set its position and size. Transparent PNGs preserve the background. The image stretches to the rectangle, so match its aspect ratio. |
| Cancel a signature | **Cancel Signature Placement** or `Esc` cancels pending or in-progress placement. After placement, select the signature and delete or move it. |
| Split / extract | Select thumbnails using `Ctrl` / `Shift`, then choose **Extract Selected Pages…**. The selected pages are saved in document order to a new PDF; the current document retains its pages. Extract different groups to split a document. |
| Reorder | Use **Move Earlier** / **Move Later** or `Alt+↑` / `Alt+↓`. Only the active page moves, even when several pages are selected. |
| Save | `Ctrl+S` saves to the current path; `Ctrl+Shift+S` saves under a different name. `*` in the window title means there are unsaved changes. |
| Insert | Choose **Blank Page…** or **Insert from PDF or DOCX…**. Positions start at 1; the page count plus 1 appends to the document. A blank page uses the adjacent page's size. Document insertion includes the entire selected PDF or all pages generated from the selected DOCX. DOCX uses the same native reader and layout limitations described below. The Word source remains unchanged. Failed or canceled insertions preserve the current document. |
| Zoom / fit | Use the View tab or menu, or the bottom zoom control. `Ctrl+0`: actual size; `Ctrl+1`: fit page; `Ctrl+2`: fit width. |

Use **Edit → Add Text…** (`Ctrl+Shift+T`), then click the page. Enter plain text (multiple lines supported), choose a font, size and color, and confirm. Esc cancels placement. Text is searchable in the saved PDF; existing PDF text is not modified. Text that exceeds the remaining page height is rejected.

Added text and signatures are editable objects. Click an object to select it (or choose **Edit → Select Text or Signature**). Drag to move the content and its selection rectangle together in real time. Drag any corner handle to resize proportionally; text keeps its line layout and remains searchable. Release to commit, or press Esc during dragging to discard the preview. Arrow keys move by 1 PDF point (Shift: 10 points). Delete/Backspace or **Delete Selected Object** removes the selected object. Esc deselects it. Objects stay within the page. Double-click an object (or select it and press F2/Enter) to edit it again. Text editing offers content, font, size and color; resizing is reflected in the displayed font size. Signature editing offers drawing on the existing signature, clearing/redrawing, or replacing its image. Signature edits retain the current position and size; text edits retain the position and width, adjusting height to fit the content. Canceling preserves the object. Changes that overflow the page are rejected. MyPdf saves the original page content and object data in an embedded attachment, alongside the normal PDF appearance; reopening in MyPdf restores selection and editing. Page reordering, insertion and extraction retain these objects. Other PDF viewers display the composed pages normally. This adds file-size overhead. Text/signatures already flattened into older or external PDFs cannot be selected with this feature.

## DOCX import for PDF editing

Open a `.docx` through **File → Open…**, the welcome screen, or a startup file argument. MyPdf directly reads the ZIP package and WordprocessingML, renders the supported content to a private temporary PDF, and immediately opens the PDF editing workspace. No Word, LibreOffice, unzip process, or external converter is used. Legacy `.doc` is deliberately unsupported.

The imported PDF starts **unsaved**, with a suggested `.pdf` name based on the Word filename. Thumbnails, zoom, signatures, page moving, insertion and extraction work immediately. **Save** (`Ctrl+S`) asks for a PDF destination the first time; subsequent saves update that PDF. **Save As…** (`Ctrl+Shift+S`) chooses another PDF path. Closing or replacing an unsaved import prompts to save. The Word original is preserved, including protection against saving over its path. A failed import preserves the current document.

The native reader supports paragraphs, headings and basic inherited styles, bold/italic/underline/strikeout, font size and color, alignment, simple bullet/decimal lists, basic tables with horizontal cell spans, and embedded PNG/JPEG/BMP/GIF/WebP images. The generated PDF uses A4 with 20 mm margins and automatic pagination; text remains searchable. External images and arbitrary document resources are not fetched.

Import uses the native reader, not Microsoft Word's pagination engine. Fonts and layout may differ. DOCX editing and saving back to DOCX are not supported. Headers/footers, footnotes/endnotes, comments, tracked-change review, complex numbering, vertical table merges, floating image placement, charts, equations and drawing objects are not fully represented. Deleted tracked text is omitted; inserted text is shown. Macros are not executed. Password-encrypted packages and ZIP64 are unsupported. Limits: 64 MiB package, 16 MiB per ZIP part, 128 MiB total declared uncompressed data, XML depth/node limits, and bounded image/table sizes. Unsupported images appear as a placeholder.

## Saving and limitations

Edits operate on private temporary copies. The source file changes only when you save; `QSaveFile` replaces the destination atomically. Failed edits preserve the current document. Temporary files are removed on normal exit. Page operations retain the original text and vector content rather than rasterizing the entire PDF.

Certificate-based signing and identity verification are not implemented. Editing a PDF with an existing digital signature may invalidate that signature.

Password-locked PDFs, general undo/redo, direct text editing, and OCR are not supported. After extracting or reordering pages, verify document-level bookmarks, internal links, and forms separately. Rendering and PDF processing currently run on the UI thread, so large documents can temporarily make the interface unresponsive.

## Project structure and tests

| Path | Purpose |
| --- | --- |
| `src/document.*` | PDF loading, rendering, page operations, signature composition/cancellation, and atomic saving. |
| `src/main.cpp` | Qt Widgets interface, dialogs, signature placement, and language settings. |
| `src/icons.*` | Monochrome vector icons, application style, and file dialog icon provider. |
| `src/i18n.*` | English source messages, translation lookup, and Qt dialog translations. |
| `translations/` | German and Korean JSON catalogs keyed by English messages, embedded through Qt resources. |
| `tests/document_test.cpp` | PDF operations, searchable text, original-file preservation, failure handling, rotation, and cropped pages. |
| `tests/localization_test.cpp` | Language switching, saved preferences, translated messages, and preservation of editing state. |
| `tests/objects_test.cpp` | Editable objects: save/reopen, move/delete, page operations, mouse/keyboard and failure atomicity. |
| `tests/signature_test.cpp` | Pending signature cancellation, object selection/deletion, and text insertion dialogs. |
| `src/toolpaths.*` | Cross-platform qpdf discovery: override, local executable, macOS helper, PATH, and configure-time fallback. |
| `tests/toolpaths_test.cpp` | Tool lookup precedence and paths containing spaces. |
| `cmake/`, `packaging/`, `.github/workflows/` | Generated build configuration, native app resources, and three-platform CI. |
| `src/docx.*` | Native bounded ZIP/WordprocessingML reader and Qt rich-text rendering; no external conversion. |
| `tests/docx_test.cpp` | Native DOCX formatting, lists, tables, images, malformed input, PDF export/pagination, and import-to-PDF editing and save behavior. |
| `scripts/` | Build/test and launch helpers. |
| `.vscode/` | CMake and IntelliSense configuration using `build/compile_commands.json`. |
| `build/` | Generated binaries, compiler outputs, and CMake files; regenerated by building. |
| `output/` | Optional generated artifacts, such as the test PDF converted from an earlier README. |

Run `./scripts/build.sh` to build and test, or `ctest --test-dir build --output-on-failure` to run the existing test build. The automated GUI tests use Qt's offscreen platform.

## Documentation maintenance

`README.md` is the English reference. `README.de.md` and `README.ko.md` contain the same instructions in German and Korean. When behavior, dependencies, or commands change, update all three in the same change. Keep their sections and examples aligned; UI translations are maintained separately in `translations/`.

Review the Qt 6, Poppler, and qpdf licenses before distributing the application. No project-wide license is currently included.
