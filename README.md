# MyPdf

**English** | [Deutsch](README.de.md) | [한국어](README.ko.md)

A C++17 / Qt 6 PDF editor for Ubuntu, Windows and macOS. Read PDFs, organize pages, and add text and signatures.

## Features

- Open, close, save and save a copy of PDFs.
- Reorder pages, extract selected pages into a new PDF, and insert blank pages, PDFs or DOCX files.
- Add text, draw signatures or import signature images.
- Select, drag, resize, double-click to edit, and delete added text/signatures.
- Import DOCX as PDF for editing and saving; no Word or LibreOffice required.
- Page thumbnails, zoom, fit page and fit width.
- English, German and Korean interfaces; light and dark themes.

## Requirements

| Category | Requirements |
| --- | --- |
| Build tools | C++17 compiler, CMake 3.22+, Make or Ninja, Bash |
| Libraries | Qt 6.4+ Widgets, Poppler **Qt 6 bindings**, zlib |
| Runtime tool | qpdf: required for page operations and text/signature composition |
| Language resources | Qt translation package for standard dialogs |
| Linux packaging | Python 3, binutils; internet for the initial tool download |
| macOS packaging | Python 3, Xcode Command Line Tools, macdeployqt from the linked Qt installation |

Obtain the source through Git or a ZIP archive and run commands from the **project root**. Install native dependencies and configure a fresh build on each computer. Do not reuse `build/`, CMake caches or executables across operating systems. Keep Qt, Poppler and the compiler on the same architecture and toolchain.

## Ubuntu setup and build

For Ubuntu 26.04, using GCC and system packages:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config qt6-base-dev libpoppler-qt6-dev qpdf qt6-translations-l10n zlib1g-dev
./scripts/build.sh
./scripts/run.sh
```

Output: `build/MyPdf`. Install for the current user, including the application menu entry and icon:

```bash
cmake --install build --prefix "$HOME/.local"
"$HOME/.local/bin/MyPdf"
```

This installation continues to use system Qt, Poppler and qpdf. To update, quit the app, rebuild, and repeat the installation command.

## Windows setup and build

1. Install [MSYS2](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** terminal and update. If asked to close the terminal, reopen UCRT64 and finish updating.

```bash
pacman -Syu
```

3. Change to the project folder, for example `cd /c/dev/MyPdf`. Install dependencies and build in that same UCRT64 terminal:

```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-qt6-base \
  mingw-w64-ucrt-x86_64-qt6-translations \
  mingw-w64-ucrt-x86_64-poppler-qt6 \
  mingw-w64-ucrt-x86_64-qpdf \
  mingw-w64-ucrt-x86_64-zlib
./scripts/build.sh
./scripts/run.sh
```

Output: `build/MyPdf.exe`. The script selects Ninja and UCRT64 library paths. **Do not mix MSYS, MINGW64 or MSVC Qt libraries.** Run from UCRT64 so DLLs and qpdf in `/ucrt64/bin` are discoverable. [MSYS2 environments](https://www.msys2.org/docs/environments/)

To install elsewhere, use the same terminal:

```bash
cmake --install build --prefix "$HOME/Apps/MyPdf"
"$HOME/Apps/MyPdf/bin/MyPdf.exe"
```

This is not a self-contained package with bundled DLLs.

## macOS setup and build

1. Install Apple developer tools and complete the installation dialog. Skip this step if already installed.

```bash
xcode-select --install
```

2. Install [Homebrew](https://brew.sh/) and apply its final `brew shellenv` instructions to your shell. `brew --version` must work in the terminal. Use native ARM dependencies on Apple Silicon or x86_64 dependencies on Intel; do not mix Rosetta installations.
3. Install dependencies and build from the project folder:

```bash
brew install cmake pkgconf qtbase qttranslations poppler-qt6 qpdf zlib python
./scripts/build.sh
./scripts/run.sh
```

`poppler-qt6` is separate from plain `poppler`. The build script configures Homebrew Qt, Poppler, zlib and qpdf paths automatically. [Homebrew package information](https://formulae.brew.sh/formula/poppler-qt6)

`vulkan-headers` is optional for Qt's header discovery, not a MyPdf runtime dependency. Install only if needed; no Vulkan SDK or runtime is required.

```bash
brew install vulkan-headers
```

Output: `build/MyPdf.app`. GUI tests use Cocoa by default and require a logged-in macOS desktop session. Install as a normal local app:

```bash
mkdir -p "$HOME/Applications"
cmake --install build --prefix "$HOME/Applications"
open "$HOME/Applications/MyPdf.app"
```

Launch by double-clicking in Finder or keeping the app in the Dock; `run.sh` is unnecessary. Keep Homebrew dependencies installed for this installation. To update, quit the app and repeat the build/install commands.

## Release build

First complete `./scripts/build.sh` for your OS, then run the following. This reuses the configured dependency paths and generator:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`build.sh` always builds Debug and runs tests. Run the commands above last when preparing a Release. By default, `run.sh` locates the executable in `build/`; set `MYPDF_BUILD_DIR` for another directory. You can also pass a document path as a launch argument.

## Deploying to another computer

### macOS: self-contained app

Run **on macOS after completing the Release build**. Use `--config Debug` for a Debug build. The packaging script does not build the project.

```bash
./scripts/package-macos.sh --config Release
open dist/MyPdf.app
```

Packaging uses Qt's [macdeployqt](https://doc.qt.io/qt-6/macos-deployment.html), includes required plugins, Poppler, qpdf and linked non-system libraries, and places qpdf under `Contents/Helpers`. It checks library paths, architectures, local signatures and app/qpdf startup, preserving the original build.

Existing output is never overwritten. Move the previous package or select another output path:

```bash
./scripts/package-macos.sh --build-dir build --config Release --output dist/MyPdf-new.app
```

Distribute the **entire `dist/MyPdf.app`**. Copy it to `~/Applications` or `/Applications` on the destination Mac. Homebrew and developer tools are unnecessary there, but the architecture and macOS version must be compatible with the build. The app uses system fonts; packaging does not create a universal binary. To create a ZIP:

```bash
ditto -c -k --sequesterRsrc --keepParent dist/MyPdf.app dist/MyPdf-macos.zip
```

Signing currently uses local ad-hoc signatures. Public Developer ID signing, notarization and DMG creation are separate tasks. Before distribution, verify opening/saving, DOCX import, and page/signature editing on a compatible Mac without Homebrew.

### Linux: single-file AppImage

Run on a **Linux computer with the Release build completed**. Additional build tools are Python 3 and binutils. Packaging requires neither root privileges nor FUSE.

```bash
sudo apt install python3 binutils
./scripts/package-linux.sh
```

Output: `dist/MyPdf-0.1.0-x86_64.AppImage`, `.sha256` and `.build.json`. Version and architecture follow the build. Qt, Poppler, qpdf, the C++ runtime and required plugins are bundled; recipients do not install these packages separately. On first use, [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy), its Qt plugin and the AppImage runtime are downloaded from official GitHub releases and checked against published SHA-256 digests. Tools are cached under `build/appimage-tools` and reused without downloading again. Initial packaging needs an internet connection.

Distribute the **single AppImage file**. Providing `.sha256` enables integrity checks; `.build.json` records the build environment, system libraries and tool versions. Recipients run these commands from the download folder:

```bash
sha256sum -c MyPdf-0.1.0-x86_64.AppImage.sha256
chmod +x MyPdf-0.1.0-x86_64.AppImage
./MyPdf-0.1.0-x86_64.AppImage
```

Run the checksum command only if the `.sha256` file was also provided. Without FUSE, extract and run without installing anything:

```bash
APPIMAGE_EXTRACT_AND_RUN=1 ./MyPdf-0.1.0-x86_64.AppImage
```

Keep the AppImage in any folder. To update, quit the app and replace the AppImage with the new version. Packaging does not overwrite existing output; select a different path when needed:

```bash
./scripts/package-linux.sh --config Release --build-dir build --output dist/MyPdf-new.AppImage
```

Native `x86_64` and `aarch64` are supported; local validation currently covers x86_64. The app uses host glibc, graphics drivers, X11/XWayland and fonts. **It does not run on every Linux version.** Build on the oldest OS you intend to support. An Ubuntu 26.04 build is not guaranteed to run on older Ubuntu releases. CI is configured to build on Ubuntu 24.04 and check startup/PDF operations in a container without Qt, Poppler or qpdf installed. Verify opening/saving, signatures and page editing on your destination OS before distribution. deb packaging and automatic updates are not provided.

### Windows: installed runtimes

On Windows, `cmake --install` installs the app without copying DLLs. Install MSYS2 UCRT64 and the packages above on the destination, then launch from UCRT64. Windows portable packaging and installers are not yet provided.

## Usage and limitations

| Task | How |
| --- | --- |
| Open and save | File → Open / Save / Save As. Opening DOCX imports it as PDF; the first save asks for a PDF destination. |
| Add text/signatures | Select Text / Sign on the toolbar and place the object on the page. |
| Edit added objects | Click to select, drag to move, resize with corner handles and double-click to edit. Arrow keys move, Shift increases the step, Delete/Backspace deletes. Esc cancels placement/movement or deselects. |
| Organize pages | Use Pages for moving, inserting and extracting. Select multiple pages in the thumbnail sidebar. |
| Language/theme | Change in Settings. Menu names here use the English interface. |

Added text/signatures remain selectable and editable after saving and reopening in MyPdf, and display in other PDF viewers. Text/signatures already flattened into existing PDFs cannot be edited individually.

DOCX import supports basic paragraphs, styles, lists, tables and images. Fonts and pagination can differ from Word; complex drawings, equations, headers and footnotes have limited support. Check the layout before saving. The Word original is preserved. Legacy `.doc` and saving back to DOCX are unsupported.

Signatures are visual, not certificate-based digital signatures. OCR, direct editing of existing PDF text, general undo/redo and locked PDFs are unsupported. Large documents can temporarily make the interface unresponsive.

## Tests and project structure

```bash
ctest --test-dir build --output-on-failure
python3 tests/package_macos_test.py
python3 tests/package_linux_test.py
```

The first command runs the built application's tests. The other commands use Python 3 to test macOS/Linux packaging logic. GUI tests default to Cocoa on macOS and offscreen elsewhere.

| Path | Purpose |
| --- | --- |
| `src/` | PDF/DOCX processing, UI, icons and language settings |
| `translations/`, `assets/` | Embedded translations and application resources |
| `scripts/`, `cmake/`, `packaging/` | Build, launch, installation and Linux/macOS packaging |
| `tests/` | Document, UI and packaging verification |
| `build/`, `dist/` | Generated build results and distribution packages |

[GitHub Actions](.github/workflows/build.yml) builds/tests on all three operating systems and retains Linux AppImage output as `MyPdf-linux-x86_64` and the macOS ZIP as `MyPdf-macos` artifacts.

## Documentation and licensing

`README.md` is the English reference; German and Korean versions keep the same structure and commands. Update all three when features, dependencies or deployment instructions change.

A project-wide license file is not yet included. Before external distribution, select a project license and review license, notice and source-availability conditions for Qt, Poppler, qpdf and bundled libraries.
