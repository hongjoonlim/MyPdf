# Third-party software notices

MyPdf uses the following independent projects. Their authors retain their
copyrights. No Adobe Acrobat code or SDK is included for AcroForm support.

| Component | License family | Project / source |
| --- | --- | --- |
| Qt 6 Widgets, Gui, Core and plugins/translations | LGPL-3.0 or GPL-3.0; included third-party parts have additional terms | https://www.qt.io/ / https://code.qt.io/ |
| Poppler and its Qt 6 bindings | GPL-2.0 or GPL-3.0, with additional file-specific notices | https://poppler.freedesktop.org/ / https://gitlab.freedesktop.org/poppler/poppler |
| qpdf | Apache-2.0; see bundled notices for incorporated code | https://github.com/qpdf/qpdf |
| AppImage type2 runtime (Linux package only), including musl, libfuse, squashfuse, zstd and zlib | MIT, LGPL and component-specific licenses; see LICENSES/AppImage-runtime.txt and accompanying texts | https://github.com/AppImage/type2-runtime |
| zlib | zlib license | https://zlib.net/ |

MyPdf uses the GPLv3-compatible licensing options of these components.
The full MyPdf GPLv3 license is in LICENSE; other principal license texts and
reference copyright notices are in LICENSES/. The reference notices originate
from the Ubuntu packages used during development; they are not an inventory of
every possible build. Exact component versions, copyrights and optional plugin
licenses depend on the machine used to create a release.

The packaging scripts additionally collect installed package notices and version
metadata for the actual Linux/Homebrew dependencies into dependency-notices/ and
DEPENDENCIES.json. Retain those files when redistributing. Packages built from
custom prefixes need their own license/source records; inspect any unresolved
entries. A link to upstream alone does not replace corresponding-source delivery.

Qt and Poppler can use further libraries (for example font, image, compression,
cryptographic and C++ runtime libraries). Their notices must also accompany a
binary containing those libraries. Full license texts for distribution-provided
notices may be included in common-licenses/ in packaged releases.

PDF/AcroForm is a document standard. MyPdf is an independent application and is
not affiliated with or endorsed by Adobe. Adobe, Acrobat and other product names
are trademarks of their respective owners.

See DISTRIBUTING.md for source provision and release requirements.
