# Distributing MyPdf

MyPdf is GPL-3.0-or-later. README is a useful introduction, but a README notice
alone does not satisfy binary distribution obligations.

## What is included

- Help → Licenses… displays the project license and reference third-party notices,
  including when the executable is moved without the accompanying files.
- CMake installation includes LICENSE, NOTICE, this document,
  THIRD_PARTY_NOTICES.md and LICENSES/. macOS installs them inside
  MyPdf.app/Contents/Resources/licenses; other systems under share/MyPdf/licenses.
- Linux/macOS packaging includes the same files, a MyPdf-source.tar.gz snapshot
  of the current application source/build scripts, and collected dependency
  notices/version metadata. The source snapshot contains working-tree changes;
  it excludes build outputs, personal configuration and Git history.

## Before sharing a binary

1. Keep all notices and license texts with the app. Review DEPENDENCIES.json and
   dependency-notices/ for the exact release. Resolve missing/custom-package
   notices and check optional plugins and embedded third-party components.
2. Provide complete corresponding source for the version of MyPdf being shared,
   including modifications and build/install scripts. The bundled application
   source snapshot covers MyPdf, not the bundled dependency source trees.
3. Also make the exact corresponding sources and patches for bundled GPL/LGPL
   dependencies available under their applicable licenses. Include the AppImage
   runtime and its statically linked LGPL components, with the materials needed
   for relinking as required by their licenses. Use the source package
   version recorded in DEPENDENCIES.json, not simply the newest upstream release.
   Keep distribution patches and packaging recipes. Homebrew receipts/formulae
   help identify sources but do not themselves contain those source trees.
4. For a download release, place the matching application and dependency source
   archives beside the binary under equally accessible download links, without
   an additional charge. Publish a release-specific source location and retain
   it for as long as that binary is distributed. Do not rely on an unversioned
   repository link or on an upstream site remaining available.
5. Preserve the GPL freedoms and do not add restrictions on modification or
   reverse engineering required to debug library modifications. Review any
   applicable installation-information requirements when distributing a User
   Product. Signing/notarization does not remove license obligations.

The scripts collect notices; they do not download all dependency sources or
certify a release's license compliance. No written source offer is made by these
files. Supply the sources through the release method above before distributing.
Local private use without redistribution does not require publishing source.

## Obtaining exact dependency sources

- Debian/Ubuntu: use the recorded source package name/version with `apt source`
  after enabling the corresponding source repositories. Retain the `.dsc`,
  original source archive, Debian/Ubuntu patches and build rules. Repositories
  may retire old versions; archive them when preparing the release.
- Homebrew: retain the installed formula (`.brew/*.rb`), `INSTALL_RECEIPT.json`,
  exact source tarballs and patches for every bundled relevant formula, including
  dependencies. A bottle is a binary, not a source archive.
- MSYS2: retain the matching PKGBUILD, sources, patches and license files from
  the exact package versions used. Windows portable packaging is not implemented;
  adding DLLs manually also requires their notices and applicable sources.

Primary guidance: https://www.gnu.org/licenses/gpl-faq.html
Qt terms: https://doc.qt.io/qt-6/licensing.html
