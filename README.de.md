# MyPdf

[English](README.md) | **Deutsch** | [한국어](README.ko.md)

Ein C++17-/Qt-6-PDF-Editor für Ubuntu, Windows und macOS. PDFs lesen, Seiten organisieren sowie Text und Unterschriften hinzufügen.

## Funktionen

- PDFs öffnen, schließen, speichern und unter anderem Namen speichern.
- Seiten verschieben, ausgewählte Seiten als PDF extrahieren und leere Seiten, PDFs oder DOCX einfügen.
- Text hinzufügen, Unterschriften zeichnen oder als Bild importieren.
- Textmarker mit einstellbarer Farbe, Breite und Deckkraft; Shift für waagerechte Striche.
- Hinzugefügte Texte/Unterschriften auswählen, verschieben, skalieren, per Doppelklick bearbeiten und löschen.
- DOCX als PDF importieren, bearbeiten und speichern; Word oder LibreOffice sind nicht erforderlich.
- Seitenvorschau; Zoom sowie Seiten-/Breitenanpassung in der oberen Werkzeugleiste.
- Englische, deutsche und koreanische Oberfläche; helles und dunkles Design.

## Voraussetzungen

| Kategorie | Erforderlich |
| --- | --- |
| Build-Werkzeuge | C++17-Compiler, CMake ab 3.22, Make oder Ninja, Bash |
| Bibliotheken | Qt Widgets ab 6.4, Poppler **mit Qt-6-Anbindung**, zlib |
| Laufzeitwerkzeug | qpdf für Seitenbearbeitung und Text-/Unterschrift-Komposition |
| Sprachressourcen | Qt-Übersetzungspaket für Standarddialoge |
| Linux-Paketierung | Python 3, binutils; Internet für den ersten Werkzeug-Download |
| macOS-Paketierung | Python 3, Xcode Command Line Tools, macdeployqt aus der verknüpften Qt-Installation |

Quellcode mit Git oder als ZIP herunterladen und Befehle im **Projekt-Hauptverzeichnis** ausführen. Auf jedem Rechner native Abhängigkeiten installieren und neu konfigurieren. `build/`, CMake-Caches und Programmdateien nicht zwischen Betriebssystemen übernehmen. Qt, Poppler und Compiler müssen dieselbe Architektur und Toolchain verwenden.

## Ubuntu: Einrichtung und Build

Für Ubuntu 26.04 mit GCC und Systempaketen:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config qt6-base-dev libpoppler-qt6-dev qpdf qt6-translations-l10n zlib1g-dev
./scripts/build.sh
./scripts/run.sh
```

Ergebnis: `build/MyPdf`. Für den aktuellen Benutzer einschließlich Programmeintrag und Symbol installieren:

```bash
cmake --install build --prefix "$HOME/.local"
"$HOME/.local/bin/MyPdf"
```

Diese Installation verwendet weiterhin Qt, Poppler und qpdf aus dem System. Zum Aktualisieren die App beenden, neu bauen und den Installationsbefehl wiederholen.

## Windows: Einrichtung und Build

1. [MSYS2](https://www.msys2.org/) installieren.
2. Das Terminal **MSYS2 UCRT64** öffnen und aktualisieren. Wenn zum Schließen aufgefordert wird, UCRT64 erneut öffnen und die Aktualisierung abschließen.

```bash
pacman -Syu
```

3. Ins Projektverzeichnis wechseln, etwa `cd /c/dev/MyPdf`. Im selben UCRT64-Terminal Abhängigkeiten installieren und bauen:

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

Ergebnis: `build/MyPdf.exe`. Das Skript setzt Ninja und die UCRT64-Bibliothekspfade. **Keine Qt-Bibliotheken aus MSYS, MINGW64 oder MSVC beimischen.** Auch den Start im UCRT64-Terminal ausführen, damit DLLs und qpdf unter `/ucrt64/bin` gefunden werden. [MSYS2-Umgebungen](https://www.msys2.org/docs/environments/)

Installation an einem anderen Ort im selben Terminal:

```bash
cmake --install build --prefix "$HOME/Apps/MyPdf"
"$HOME/Apps/MyPdf/bin/MyPdf.exe"
```

Dies ist kein eigenständiges Paket mit eingebetteten DLLs.

## macOS: Einrichtung und Build

1. Apple-Entwicklerwerkzeuge installieren und den Installationsdialog abschließen. Bei vorhandener Installation überspringen.

```bash
xcode-select --install
```

2. [Homebrew](https://brew.sh/) installieren und die abschließenden Hinweise zu `brew shellenv` in der Shell umsetzen. `brew --version` muss im Terminal funktionieren. Auf Apple Silicon native ARM-Abhängigkeiten, auf Intel x86_64-Abhängigkeiten verwenden; keine Rosetta-Installationen beimischen.
3. Im Projektverzeichnis Abhängigkeiten installieren und bauen:

```bash
brew install cmake pkgconf qtbase qttranslations poppler-qt6 qpdf zlib python
./scripts/build.sh
./scripts/run.sh
```

`poppler-qt6` ist ein separates Paket neben `poppler`. Das Build-Skript setzt die Homebrew-Pfade für Qt, Poppler, zlib und qpdf automatisch. [Homebrew-Paketinformationen](https://formulae.brew.sh/formula/poppler-qt6)

`vulkan-headers` ist optional für die Qt-Headersuche und keine MyPdf-Laufzeitabhängigkeit. Nur bei Bedarf installieren; Vulkan-SDK und Vulkan-Laufzeit sind nicht erforderlich.

```bash
brew install vulkan-headers
```

Ergebnis: `build/MyPdf.app`. GUI-Tests verwenden standardmäßig Cocoa und benötigen eine angemeldete macOS-Desktopsitzung. Als normale lokale App installieren:

```bash
mkdir -p "$HOME/Applications"
cmake --install build --prefix "$HOME/Applications"
open "$HOME/Applications/MyPdf.app"
```

Die App im Finder doppelklicken oder im Dock behalten; `run.sh` ist nicht erforderlich. Für diese Installation müssen die Homebrew-Abhängigkeiten erhalten bleiben. Zur Aktualisierung die App beenden und Build/Installation wiederholen.

## Release-Build

Zuerst `./scripts/build.sh` für das jeweilige Betriebssystem abschließen. Anschließend die gespeicherten Abhängigkeitspfade und den Generator weiterverwenden:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`build.sh` baut immer Debug und führt Tests aus. Für eine Release-Ausgabe zuletzt die obigen Befehle ausführen. `run.sh` sucht standardmäßig unter `build/`; für andere Verzeichnisse `MYPDF_BUILD_DIR` setzen. Ein Dokumentpfad kann als Startargument übergeben werden.

## Auf einem anderen Rechner bereitstellen

### macOS: eigenständige App

**Unter macOS nach abgeschlossenem Release-Build** ausführen. Für Debug `--config Debug` verwenden. Das Paketierungsskript baut das Projekt nicht.

```bash
./scripts/package-macos.sh --config Release
open dist/MyPdf.app
```

Die Paketierung verwendet Qt [macdeployqt](https://doc.qt.io/qt-6/macos-deployment.html), integriert benötigte Plugins, Poppler, qpdf und verknüpfte Nicht-Systembibliotheken. qpdf liegt unter `Contents/Helpers`. Bibliothekspfade, Architekturen, lokale Signaturen sowie App-/qpdf-Start werden geprüft; der ursprüngliche Build bleibt erhalten.

Vorhandene Ausgaben werden nicht überschrieben. Das vorherige Paket verschieben oder einen anderen Ausgabepfad wählen:

```bash
./scripts/package-macos.sh --build-dir build --config Release --output dist/MyPdf-new.app
```

Die **gesamte `dist/MyPdf.app`** weitergeben und auf dem Ziel-Mac nach `~/Applications` oder `/Applications` kopieren. Homebrew und Entwicklerwerkzeuge werden dort nicht benötigt; Architektur und macOS-Version müssen jedoch zum Build passen. Systemschriften werden verwendet; es entsteht keine Universal-App. ZIP erstellen:

```bash
ditto -c -k --sequesterRsrc --keepParent dist/MyPdf.app dist/MyPdf-macos.zip
```

Die Signierung verwendet derzeit lokale Ad-hoc-Signaturen. Öffentliche Developer-ID-Signierung, Notarisierung und DMG-Erstellung sind separate Aufgaben. Vor der Weitergabe Öffnen/Speichern, DOCX-Import sowie Seiten-/Unterschrift-Bearbeitung auf einem kompatiblen Mac ohne Homebrew prüfen.

### Linux: einzelne AppImage-Datei

Auf einem **Linux-Rechner nach abgeschlossenem Release-Build** ausführen. Zusätzlich werden Python 3 und binutils benötigt. Die Paketierung erfordert weder Administratorrechte noch FUSE.

```bash
sudo apt install python3 binutils
./scripts/package-linux.sh
```

Ergebnis: `dist/MyPdf-0.2.0-x86_64.AppImage`, `.sha256` und `.build.json`. Version und Architektur richten sich nach dem Build. Qt, Poppler, qpdf, C++-Laufzeit und erforderliche Plugins werden integriert; Empfänger müssen diese Pakete nicht separat installieren. [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy), sein Qt-Plugin und die AppImage-Laufzeit werden beim ersten Aufruf aus offiziellen GitHub-Releases heruntergeladen und anhand veröffentlichter SHA-256-Werte geprüft. Die Werkzeuge werden unter `build/appimage-tools` zwischengespeichert und danach ohne erneuten Download verwendet. Die erste Paketierung benötigt Internetzugang.

Die **einzelne AppImage-Datei** weitergeben. Mit `.sha256` lässt sich die Integrität prüfen; `.build.json` dokumentiert Build-Umgebung, Systembibliotheken und Werkzeugversionen. Empfänger führen im Download-Verzeichnis aus:

```bash
sha256sum -c MyPdf-0.2.0-x86_64.AppImage.sha256
chmod +x MyPdf-0.2.0-x86_64.AppImage
./MyPdf-0.2.0-x86_64.AppImage
```

Die Prüfsummenprüfung nur ausführen, wenn auch `.sha256` mitgeliefert wurde. Ohne FUSE ist ein Start durch temporäres Entpacken ohne Installation möglich:

```bash
APPIMAGE_EXTRACT_AND_RUN=1 ./MyPdf-0.2.0-x86_64.AppImage
```

Das AppImage kann in einem beliebigen Ordner liegen. Zum Aktualisieren die App beenden und durch die neue Version ersetzen. Vorhandene Pakete werden nicht überschrieben; bei Bedarf einen neuen Ausgabepfad wählen:

```bash
./scripts/package-linux.sh --config Release --build-dir build --output dist/MyPdf-new.AppImage
```

Native `x86_64` und `aarch64` werden unterstützt; lokal wurde bisher x86_64 geprüft. glibc, Grafiktreiber, X11/XWayland und Schriften stammen vom Zielsystem. **Nicht jede Linux-Version ist kompatibel.** Auf dem ältesten zu unterstützenden Betriebssystem bauen. Ein Ubuntu-26.04-Build läuft nicht garantiert auf älteren Ubuntu-Versionen. Die CI ist für einen Ubuntu-24.04-Build sowie Start-/PDF-Prüfungen in einem Container ohne installiertes Qt, Poppler oder qpdf eingerichtet. Öffnen/Speichern, Unterschriften und Seitenbearbeitung auf dem Zielsystem vor Weitergabe prüfen. deb-Paketierung und automatische Updates werden nicht angeboten.

#### Im Benutzerverzeichnis ohne Versionsnummer installieren

Nach Release-Build und AppImage-Paketierung im Projektverzeichnis die App nach `~/.local/bin/MyPdf` kopieren. Eine laufende MyPdf-App vorher beenden. Administratorrechte sind nicht erforderlich; Ausführungsrechte werden mitgesetzt. Den Quelldateinamen an die tatsächlich erzeugte Version und Architektur anpassen.

```bash
install -Dm755 dist/MyPdf-0.2.0-x86_64.AppImage "$HOME/.local/bin/MyPdf"
"$HOME/.local/bin/MyPdf"
```

Ein AppImage läuft auch ohne Dateiendung. Ist `~/.local/bin` in `PATH` enthalten, lässt sich `MyPdf` aus jedem Verzeichnis starten. Andernfalls Folgendes ausführen und die `export`-Zeile für eine dauerhafte Einstellung in die Startdatei der Shell aufnehmen, etwa `~/.bashrc` für Bash:

```bash
export PATH="$HOME/.local/bin:$PATH"
MyPdf
```

Zum Aktualisieren die App beenden und das neue AppImage mit demselben Installationsbefehl kopieren. Ohne FUSE `APPIMAGE_EXTRACT_AND_RUN=1 "$HOME/.local/bin/MyPdf"` verwenden.

### Windows: installierte Laufzeit

Unter Windows installiert `cmake --install` die App ohne DLLs. Auf dem Zielrechner MSYS2 UCRT64 und die oben genannten Pakete installieren und die App aus UCRT64 starten. Portable Windows-Pakete und Installationsprogramme werden noch nicht angeboten.

## Bedienung und Grenzen

Die oberen Schaltflächen für Unterschrift, Markieren, Seiten und Anpassung öffnen ihr Menü einheitlich beim Klick auf die Schaltfläche oder ihren Pfeil.

| Aufgabe | Bedienung |
| --- | --- |
| Öffnen und speichern | File → Open / Save / Save As. DOCX wird als PDF importiert; beim ersten Speichern den PDF-Zielpfad wählen. |
| Text/Unterschriften hinzufügen | Text / Sign in der Werkzeugleiste wählen und auf der Seite platzieren. |
| Markieren | Textmarker im oberen Menü Markieren aktivieren und über die Seite ziehen. Im selben Menü Farbe, Strichbreite und Deckkraft einstellen; Shift zeichnet waagerechte Striche. Esc beendet das Werkzeug. Markierungen mit Select auswählen, verschieben/löschen oder per Doppelklick bearbeiten. |
| Hinzugefügte Objekte bearbeiten | Anklicken zum Auswählen, ziehen zum Verschieben, Eckgriffe zum Skalieren, Doppelklick zum Bearbeiten. Pfeiltasten verschieben, Shift vergrößert den Schritt, Delete/Backspace löscht. Esc bricht Platzierung/Verschieben ab oder hebt die Auswahl auf. |
| Seiten organisieren | Pages zum Verschieben, Einfügen und Extrahieren verwenden. Mehrfachauswahl in der Seitenvorschau. |
| Sprache/Design | In Settings ändern. Die Menünamen hier entsprechen der englischen Oberfläche. |

Hinzugefügte Texte, Unterschriften und Markierungen bleiben nach Speichern und erneutem Öffnen in MyPdf bearbeitbar und werden in anderen PDF-Betrachtern angezeigt. Bereits in bestehende PDFs eingebettete Texte/Unterschriften sind nicht einzeln bearbeitbar.

DOCX-Import unterstützt grundlegende Absätze, Stile, Listen, Tabellen und Bilder. Schriften und Seitenumbrüche können von Word abweichen; komplexe Zeichnungen, Formeln, Kopfzeilen und Fußnoten sind eingeschränkt. Layout vor dem Speichern prüfen. Die Word-Originaldatei bleibt erhalten. Altes `.doc` und Rückspeichern als DOCX werden nicht unterstützt.

Unterschriften sind visuell, keine zertifikatsbasierten digitalen Signaturen. OCR, direkte Bearbeitung vorhandenen PDF-Texts, allgemeines Rückgängig/Wiederholen und gesperrte PDFs werden nicht unterstützt. Große Dokumente können die Oberfläche vorübergehend blockieren.

## Tests und Projektstruktur

```bash
ctest --test-dir build --output-on-failure
python3 tests/package_macos_test.py
python3 tests/package_linux_test.py
```

Der erste Befehl führt die Tests des gebauten Programms aus. Die weiteren Befehle prüfen mit Python 3 die macOS-/Linux-Paketierungslogik. GUI-Tests verwenden standardmäßig Cocoa unter macOS und offscreen auf anderen Systemen.

| Pfad | Zweck |
| --- | --- |
| `src/` | PDF-/DOCX-Verarbeitung, Oberfläche, Symbole und Spracheinstellungen |
| `translations/`, `assets/` | Eingebettete Übersetzungen und App-Ressourcen |
| `scripts/`, `cmake/`, `packaging/` | Build, Start, Installation und Linux-/macOS-Paketierung |
| `tests/` | Dokument-, Oberflächen- und Paketierungsprüfung |
| `build/`, `dist/` | Generierte Build-Ergebnisse und Distributionspakete |

[GitHub Actions](.github/workflows/build.yml) baut/testet unter allen drei Betriebssystemen und speichert Linux-AppImage-Dateien als `MyPdf-linux-x86_64` und das macOS-ZIP als `MyPdf-macos`.

## Dokumentation und Lizenz

`README.md` ist die englische Referenz. Deutsche und koreanische Fassungen behalten dieselbe Struktur und dieselben Befehle. Bei Änderungen an Funktionen, Abhängigkeiten oder Bereitstellung alle drei aktualisieren.

Eine projektweite Lizenzdatei fehlt derzeit. Vor externer Weitergabe eine Projektlizenz festlegen und Lizenz-, Hinweis- und Quellcodebedingungen von Qt, Poppler, qpdf und mitgelieferten Bibliotheken prüfen.
