# MyPdf

[English](README.md) | **Deutsch** | [한국어](README.ko.md)

Ein PDF-Editor mit C++17 und Qt 6 für Ubuntu, Windows und macOS. Lesen Sie Dokumente, organisieren Sie Seiten und fügen Sie handgezeichnete Unterschriften oder Unterschriftsbilder hinzu. Poppler übernimmt die PDF-Darstellung, qpdf die Seitenbearbeitung.

## Funktionen

- PDF-Dateien öffnen und schließen.
- DOCX-Dokumente als bearbeitbare PDF importieren; das alte DOC-Format wird nicht unterstützt.
- Unterschriften zeichnen oder als Bild importieren.
- Dokumente durch Extrahieren ausgewählter Seiten in separate PDFs aufteilen.
- Seiten im Dokument nach vorne oder hinten verschieben.
- Änderungen speichern oder unter einem anderen Namen speichern.
- Leere Seiten, sämtliche Seiten einer anderen PDF oder alle aus DOCX erzeugten Seiten einfügen.
- Ohne Neustart zwischen englischer, deutscher und koreanischer Oberfläche wechseln.
- Zwischen heller und dunkler Darstellung wechseln; die Auswahl wird gespeichert.

Unterschriften sind **visuelle Unterschriften**, keine zertifikatsbasierten digitalen Signaturen.

Als Anwendungssymbol dient der ausgewählte Pomeranian-Entwurf (08), gespeichert unter `assets/icons/MyPdf.png` und in die Programmdatei eingebettet. Bei der Linux-Installation werden auch `packaging/MyPdf.desktop` und das Symbol installiert. Mit `cmake --install build --prefix "$HOME/.local"` im Projektverzeichnis installieren Sie die Anwendung für den aktuellen Benutzer; Der installierte Desktop-Eintrag verwendet absolute Programm- und Symbolpfade; der Start aus dem Anwendungsmenü benötigt keinen zusätzlichen PATH-Eintrag. Das Symbol wird auch im hicolor-Thema installiert; verfügbare Desktop- und Symbol-Caches werden aktualisiert.

Für die Linux-Entwicklung registriert `./scripts/install-launcher.sh` nach dem Kompilieren den aktuellen Build und das Symbol im persönlichen Anwendungsmenü, ohne die Programmdatei zu kopieren. Beenden Sie MyPdf und starten Sie es über das Anwendungsmenü neu. Entfernen Sie gegebenenfalls einen alten Dock-Favoriten und heften Sie die Anwendung erneut an. Führen Sie das Skript nach einem Umzug des Projekts erneut aus. Der Eintrag wird unter `${XDG_DATA_HOME:-$HOME/.local/share}` angelegt.

## Kompilieren und starten

Benötigt werden CMake ab 3.22, ein C++17-Compiler, Qt Widgets ab 6.4, die Qt-6-Anbindung von Poppler, qpdf und zlib. Kompilieren Sie nativ auf dem jeweiligen Betriebssystem mit passenden Architekturen und Compilern. Verwenden Sie kein `build/`-Verzeichnis eines anderen Betriebssystems und wechseln Sie dessen CMake-Generator nicht nachträglich.

| Plattform | Konfiguration | Prüfstatus |
| --- | --- | --- |
| Ubuntu 26.04 | GCC, Systembibliotheken Qt / Poppler / qpdf | Nach diesen Änderungen lokal kompiliert und getestet. |
| Windows x64 | MSYS2 UCRT64, GCC, Ninja | Build-Konfiguration und native CI hinzugefügt; in dieser Sitzung nicht unter Windows ausgeführt. |
| macOS | Apple Clang, Homebrew, native Architektur | Build-Konfiguration und native CI hinzugefügt; in dieser Sitzung nicht unter macOS ausgeführt. |

### Ubuntu

Im Projektverzeichnis ausführen:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config qt6-base-dev libpoppler-qt6-dev qpdf qt6-translations-l10n zlib1g-dev
./scripts/build.sh
./scripts/run.sh
```

### Windows: MSYS2 UCRT64

Installieren Sie [MSYS2](https://www.msys2.org/), öffnen Sie das **UCRT64**-Terminal und wechseln Sie ins Projektverzeichnis, zum Beispiel mit `cd /c/dev/MyPdf`. Führen Sie die folgenden Befehle aus. Falls das erste Update zum Schließen des Terminals auffordert, öffnen Sie UCRT64 erneut und schließen Sie das Update vor der Paketinstallation ab.

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-translations mingw-w64-ucrt-x86_64-poppler-qt6 mingw-w64-ucrt-x86_64-qpdf mingw-w64-ucrt-x86_64-zlib
./scripts/build.sh
./scripts/run.sh
```

Ergebnis: `build/MyPdf.exe` mit dem Pomeranian-Symbol als `.ico`-Ressource. Starten Sie aus UCRT64, damit die DLLs von Qt, Poppler und dem Compiler über `PATH` verfügbar sind. Dokumentiert ist die MinGW-Konfiguration; mischen Sie diese Bibliotheken nicht mit einer MSVC-Qt-Installation. Paketinformationen: [Poppler Qt 6 für UCRT64](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-poppler-qt6).

### macOS: Homebrew

Installieren Sie zuerst [Homebrew](https://brew.sh/). Führen Sie im Projektverzeichnis die folgenden Befehle aus. Überspringen Sie `xcode-select --install`, wenn die Command Line Tools bereits vorhanden sind.

```bash
xcode-select --install
brew install cmake ninja pkgconf qtbase qttranslations poppler-qt6 qpdf zlib vulkan-headers
./scripts/build.sh
./scripts/run.sh
```

`build.sh` übergibt automatisch die Homebrew-Präfixe von Qt und Poppler. Benötigt wird **poppler-qt6**, nicht nur `poppler`, da die Qt-6-Anbindung erforderlich ist. Die Formel ist keg-only; siehe [Homebrew-Paketbeschreibung](https://formulae.brew.sh/formula/poppler-qt6). Ergebnis: `build/MyPdf.app` mit der Programmdatei `build/MyPdf.app/Contents/MacOS/MyPdf` und dem Pomeranian-Symbol im `.icns`-Format. Verwenden Sie für Apple Silicon bzw. Intel jeweils native Abhängigkeiten; es wird keine Universal-Binärdatei erzeugt.

#### macOS: `Could not find POPPLER_INCLUDE_DIR ... poppler-qt6.h`

Hier fehlen die **erforderlichen Poppler-Qt-6-Entwicklungsheader**, im Unterschied zur optionalen Vulkan-Suche oben. Das normale `poppler` genügt nicht. Homebrews separates [poppler-qt6-Paket](https://formulae.brew.sh/formula/poppler-qt6) ist keg-only und liegt möglicherweise außerhalb der Standardsuchpfade.

```bash
brew install poppler-qt6 pkgconf
brew list --versions poppler-qt6
ls "$(brew --prefix poppler-qt6)/include/poppler/qt6/poppler-qt6.h"
./scripts/build.sh
```

Das aktualisierte `build.sh` prüft Header und Bibliothek und übergibt Homebrews pkg-config-Pfad sowie `POPPLER_INCLUDE_DIR` und `POPPLER_LIBRARY` ausdrücklich. Auch das Skript im Mac-Checkout aktualisieren. Wird das Paket angezeigt, aber der Header fehlt, hilft `brew reinstall poppler-qt6`. Qt, Poppler und Compiler müssen dieselbe Architektur verwenden; Apple-Silicon- und Intel/Rosetta-Homebrew nicht mischen.

Bei manueller CMake-Konfiguration:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)" \
  -DPOPPLER_INCLUDE_DIR="$(brew --prefix poppler-qt6)/include/poppler/qt6" \
  -DPOPPLER_LIBRARY="$(brew --prefix poppler-qt6)/lib/libpoppler-qt6.dylib"
cmake --build build --parallel
```

#### macOS: `Tests require qpdf on PATH or -DQPDF_EXECUTABLE=...`

CMake findet die **qpdf-Programmdatei** nicht. qpdf wird sowohl für Tests als auch zur Laufzeit für Seitenbearbeitung, Unterschriften und Textkomposition benötigt; Tests abzuschalten genügt nicht. [Homebrews qpdf-Paket](https://formulae.brew.sh/formula/qpdf) installieren und prüfen:

```bash
brew install qpdf
"$(brew --prefix qpdf)/bin/qpdf" --version
./scripts/build.sh
```

Das aktualisierte `build.sh` prüft `MYPDF_QPDF`, Homebrews `qpdf/bin/qpdf` und danach `PATH` und übergibt `-DQPDF_EXECUTABLE` an CMake. Auch das Skript im Mac-Checkout aktualisieren. Schlägt der Versionsaufruf trotz Installation fehl, `brew reinstall qpdf` verwenden. Eine gesetzte Variable `MYPDF_QPDF` muss eine gültige Programmdatei bezeichnen oder entfernt werden. Bei manueller Konfiguration `-DQPDF_EXECUTABLE="$(brew --prefix qpdf)/bin/qpdf"` zum bestehenden CMake-Befehl hinzufügen.

#### macOS: mehrere GUI-Tests enden mit `Subprocess aborted`

`native_docx`, `pdf_operations`, `localization`, `signature_cancellation`, `editable_objects` und `object_editing` erzeugen eine `QApplication`. Besteht nur `tool_discovery`, ist eine gemeinsame GUI-Initialisierungsstörung, etwa beim Laden des Qt-Plattform-Plugins, plausibel. Die Fehlerliste allein belegt die Ursache nicht. Erfolgreiches Kompilieren und bestandene Tests sind getrennt zu betrachten.

Die bisherige Konfiguration erzwang `offscreen` für alle GUI-Tests und konnte unter macOS mit `Could not find the Qt platform plugin "offscreen"` abbrechen. Jetzt gilt standardmäßig `cocoa` unter macOS und `offscreen` auf anderen Systemen. Ist das passende Qt-Plugin-CMake-Target vorhanden, wird dessen tatsächlicher Pfad über `QT_QPA_PLATFORM_PLUGIN_PATH` und `QT_PLUGIN_PATH` nur an Tests übergeben. Globale Shell-Einstellungen bleiben unverändert.

`CMakeLists.txt`, `cmake/test_environment.cmake` und `scripts/build.sh` im Mac-Checkout aktualisieren und Folgendes ausführen. Cocoa-Tests benötigen eine angemeldete macOS-Desktopsitzung und können kurz Fenster oder Dialoge anzeigen.

```bash
MYPDF_TEST_QPA_PLATFORM=cocoa ./scripts/build.sh
```

Für Tests ohne Bildschirm mit installiertem Offscreen-Plugin stattdessen `MYPDF_TEST_QPA_PLATFORM=offscreen` verwenden. Ein manuell gesetztes `MYPDF_TEST_QT_PLUGIN_DIR` muss auf das `platforms`-Verzeichnis derselben Qt-Version und Architektur zeigen. Das Skript übergibt beide Variablen als CMake-Optionen; die Auswahl bleibt im Build-Cache erhalten.

Bei weiterem Abbruch die Plugin-Diagnose abrufen:

```bash
QT_DEBUG_PLUGINS=1 ctest --test-dir build -R '^pdf_operations$' -V
```

Das vollständige Protokoll steht unter `build/Testing/Temporary/LastTest.log`. Anhand tatsächlicher Meldungen wie `Could not find/load the Qt platform plugin`, `Library not loaded` oder `incompatible architecture` diagnostizieren. Prüfen, ob Plugin-Pfade auf eine andere Qt-/Python-Installation zeigen und ob Qt und Plugins dieselbe Version und Architektur verwenden. `QT_DEBUG_PLUGINS` ist eine [offizielle Qt-Diagnoseoption](https://doc.qt.io/qt-6/deployment-plugins.html). Testspezifische CTest-Umgebungswerte überschreiben die Elternumgebung: Nur `QT_QPA_PLATFORM=cocoa` in der Shell zu setzen ändert diese Konfiguration nicht. Tests abzuschalten oder ohne Protokoll weitere Pakete zu installieren belegt keine Behebung.

#### macOS: ein Test wartet im Dateidialog `Signature Image`

Die Tests benötigen keine manuelle Dateiauswahl. Qt ändert mit `selectFile()` das Dateinamenfeld nicht, wenn dieses den Fokus hat. Der bisherige sofortige Auswahl-/Bestätigungsaufruf konnte den Dialog daher offen lassen. Zusätzlich lädt das Dateisystemmodell asynchron. Aktualisierte Tests warten auf Dialog/Modell, füllen das Dateinamenfeld und bestätigen über die echte Dateiauswahlprüfung. Für die Automatisierung gelten 10 Sekunden, pro GUI-Test 120 Sekunden Zeitlimit.

`tests/file_dialog_automation.h`, `tests/object_edit_test.cpp`, `tests/docx_test.cpp` und `CMakeLists.txt` im Mac-Checkout aktualisieren und neu bauen:

```bash
MYPDF_TEST_QPA_PLATFORM=cocoa ./scripts/build.sh
ctest --test-dir build -R '^(object_editing|native_docx)$' -V --timeout 120
```

Nichts manuell eingeben. Bei einem Fehler Dateipfad und aktiven Fenstertitel in der Zeitüberschreitungsmeldung sowie `LastTest.log` prüfen. Geändert wird die Testautomatisierung; in der Anwendung wählen Benutzer ihre Unterschriftsbilder weiterhin selbst aus.

#### macOS: Tests bestehen, aber beim Start fehlt das `cocoa`-Plugin

Test-Plugin-Pfade und App-Starteinstellungen sind getrennt. CMake schreibt jetzt den Cocoa-Plugin-Pfad der verknüpften Qt-Installation nach `MyPdf.app/Contents/Resources/qt.conf`. Qt liest [diese Bundle-Konfiguration](https://doc.qt.io/qt-6/qt-conf.html) sowohl beim Start über `run.sh` als auch beim direkten App-Start. `CMakeLists.txt` und `cmake/macos_runtime.cmake` auf dem Mac aktualisieren und neu bauen:

```bash
./scripts/build.sh
./scripts/run.sh
```

Eine bereits installierte App durch das neu gebaute `.app` ersetzen. Diese lokale Entwicklungskonfiguration verweist auf Qt auf diesem Mac. Für die Weitergabe der App an andere Macs müssen Qt-Bibliotheken und Plugins separat mitgeliefert werden. Bei weiteren Fehlern die tatsächlichen Plugin-Pfade und Ladefehler mit `QT_DEBUG_PLUGINS=1 ./scripts/run.sh` prüfen.

#### macOS: `Could NOT find WrapVulkanHeaders (missing Vulkan_INCLUDE_DIR)`

Qt findet optionale Vulkan-Header nicht; diese einzelne Meldung bedeutet nicht zwingend einen Build-Abbruch. MyPdf verwendet Qt Widgets, QPainter und Poppler und ruft Vulkan nicht direkt auf. Erscheinen danach `Configuring done` und `Generating done`, war die Konfiguration erfolgreich. Bei einem Abbruch auch die folgenden `CMake Error`- und Abhängigkeitsmeldungen prüfen. Qt kennzeichnet diese Abhängigkeit im [Konfigurationsquelltext als optional](https://github.com/qt/qtbase/blob/6.10/src/gui/configure.cmake#L74-L75).

Der macOS-Installationsbefehl enthält dafür [Homebrews `vulkan-headers`](https://formulae.brew.sh/formula/vulkan-headers). Nach Installation mit einer älteren README:

```bash
brew install vulkan-headers
./scripts/build.sh
```

`build.sh` erkennt die installierten Homebrew-Header und übergibt `Vulkan_INCLUDE_DIR` automatisch. Bei manueller Konfiguration:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)" \
  -DVulkan_INCLUDE_DIR="$(brew --prefix vulkan-headers)/include"
cmake --build build --parallel
```

Für diese Meldung genügt das Header-Paket. MyPdf benötigt dafür weder ein Vulkan-SDK noch Vulkan-Loader oder MoltenVK-Laufzeit. Andere Qt-/Poppler-Abhängigkeitsfehler müssen gesondert behoben werden.


### Gemeinsame Befehle und Installation

`build.sh` erstellt einen Debug-Build und führt die Tests aus. `run.sh` wählt die passende Programmdatei. Für einen Release-Build verwenden Sie die folgenden Befehle. Ergänzen Sie unter Windows beim Konfigurieren `-G Ninja -DCMAKE_PREFIX_PATH="$MINGW_PREFIX"`, unter macOS `-DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)"`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./scripts/run.sh '/path/to/document.pdf'
```

`run.sh` sucht relativ zum Skript und ändert das Arbeitsverzeichnis nicht. Relative Dokumentpfade beziehen sich damit auf das Verzeichnis des Aufrufs. Unter macOS und Windows werden auch Konfigurationsordner wie `build/Debug` und `build/Release` unterstützt. Andere Build-Ordner lassen sich mit `MYPDF_BUILD_DIR` angeben (relativ zum aktuellen Verzeichnis oder absolut). Build-Verzeichnisse und CMake-Caches müssen auf jedem Rechner und Betriebssystem neu erzeugt werden.

Eine Entwicklungsinstallation erfolgt mit `cmake --install build --prefix /path/to/install`. macOS installiert `MyPdf.app`, Windows `bin/MyPdf.exe`; Linux installiert zusätzlich Desktop-Eintrag und Symbol. Dies sind **keine eigenständig verteilbaren Pakete**: Qt-Plugins und Bibliotheken, Poppler-Abhängigkeiten sowie qpdf mit seinen Abhängigkeiten müssen verfügbar bleiben. Automatische DLL-/dylib-Bereitstellung, Installationsprogramme sowie macOS-Signierung und Notarisierung sind nicht implementiert.

Die qpdf-Suche erfolgt in dieser Reihenfolge: `MYPDF_QPDF`, neben der Anwendung, `../Helpers/qpdf` bzw. `qpdf.exe`, `PATH` und zuletzt der beim Konfigurieren gefundene Pfad. Der letzte Pfad ist eine an den Build-Rechner gebundene Entwicklungshilfe. Für einen Finder-Start muss qpdf an einem unterstützten Ort liegen; alternativ starten Sie mit `run.sh` aus einer eingerichteten Shell. Mit `MYPDF_QPDF` können Sie den Programmpfad ohne zusätzliche Argumente ausdrücklich festlegen.

Der Workflow [`.github/workflows/build.yml`](.github/workflows/build.yml) kompiliert und testet nach dem Hochladen auf GitHub nativ unter Ubuntu, Windows UCRT64 und macOS. Das Hinzufügen der Datei bedeutet nicht, dass diese entfernten Jobs bereits erfolgreich ausgeführt wurden.

## Arbeitsoberfläche

- **Menüleiste:** Datei, Bearbeiten, Ansicht, Seiten, Einstellungen und Hilfe. Text und Unterschriften liegen gemeinsam unter Bearbeiten; Seitenoperationen unter Seiten.
- **Hauptwerkzeugleiste:** Öffnen, Speichern, Dateiname sowie Auswahl, Text, Unterschreiben und Seiten in einer Zeile. Unterschreiben öffnet direkt die Zeichenfläche; der Pfeil bietet den Bildimport. Seiten enthält Verschieben, Einfügen und Extrahieren. Speichern unter und Schließen bleiben im Dateimenü.
- **Kontextaktionen:** Bearbeiten und Löschen erscheinen nur bei ausgewähltem Objekt. Rechtsklick bietet dieselben Aktionen, Doppelklick öffnet den Editor. Abbrechen erscheint nur während der Platzierung. Aufgabenreiter entfallen.
- **Linke Seitenleiste:** Seitenvorschauen mit Mehrfachauswahl und Kontextmenü zur Seitenorganisation. Ein- und Ausblenden über Ansicht → Seitenvorschau.
- **Untere Navigation:** Vorige/nächste Seite, Seitennummer, Verkleinern/Vergrößern, 40–300 % Zoom und Anpassungsmenü (Seite, Breite, Originalgröße). Dokumente öffnen zunächst seitenfüllend.

Die Anwendung verwendet einheitliche einfarbige Kontursymbole, auch in Dateidialogen und Meldungsfenstern.

## Sprache der Oberfläche

Wählen Sie **Settings → Language → English / Deutsch / 한국어**. Der deutsche Menüpfad lautet **Einstellungen → Sprache**, der koreanische **설정 → 언어**.

Menüs, Werkzeugleisten, Seitenbeschriftungen und Anwendungsmeldungen ändern sich sofort. Dokument, ungespeicherte Änderungen, Seitenauswahl, Zoom und aktiver Werkzeugmodus bleiben erhalten.

Englisch ist die Standard- und Rückfallsprache. Ihre Auswahl wird über Qt `QSettings` als `ui/language` gespeichert; eine vorhandene Einstellung hat Vorrang. Unter Ubuntu lautet der Standardpfad `~/.config/MyPdf/MyPdf.conf`, abhängig von `XDG_CONFIG_HOME`.

Für die Qt-Standarddialoge wird das oben genannte Übersetzungspaket der jeweiligen Plattform benötigt (Ubuntu: `qt6-translations-l10n`). Ohne dieses Paket können einige Standardtexte englisch bleiben. Die Dateiauswahl verwendet Qt-Dialoge, damit sie der gewählten Anwendungssprache folgt.

## Darstellung

Wählen Sie **Einstellungen → Darstellung → Hell / Dunkel**, um die Oberfläche sofort umzustellen. Die Auswahl wird über Qt `QSettings` als `ui/theme` gespeichert; standardmäßig ist die helle Darstellung aktiviert. PDF-Seiten behalten ihre Dokumentfarben.

## Bedienung

Die Tastenkürzel unten gelten für Ubuntu/Windows. Unter macOS verwenden die entsprechenden Qt-Standardkürzel die Command-Taste.

| Aufgabe | Vorgehensweise |
| --- | --- |
| Öffnen / schließen | Menü Datei oder `Ctrl+O` / `Ctrl+W` (Strg-Taste). Bei ungespeicherten Änderungen erscheint eine Speicherabfrage. |
| Unterschrift hinzufügen | **Unterschrift zeichnen…** oder **Unterschriftsbild importieren…** wählen und ein Rechteck auf der Seite ziehen, um Position und Größe festzulegen. Transparente PNGs erhalten den Hintergrund. Das Bild wird an das Rechteck angepasst; achten Sie auf das Seitenverhältnis. |
| Unterschrift abbrechen | **Platzierung abbrechen** oder `Esc` bricht eine vorbereitete oder laufende Platzierung ab. Danach die Unterschrift zum Löschen oder Verschieben auswählen. |
| Aufteilen / extrahieren | Vorschauen mit `Ctrl` / `Shift` (Strg / Umschalt) auswählen, dann **Ausgewählte Seiten extrahieren…** wählen. Die Seiten werden in Dokumentreihenfolge in einer neuen PDF gespeichert; im aktuellen Dokument bleiben sie erhalten. Extrahieren Sie verschiedene Gruppen, um ein Dokument aufzuteilen. |
| Reihenfolge ändern | **Nach vorne verschieben** / **Nach hinten verschieben** oder `Alt+↑` / `Alt+↓`. Auch bei Mehrfachauswahl wird nur die aktive Seite verschoben. |
| Speichern | `Ctrl+S` speichert am aktuellen Pfad; `Ctrl+Shift+S` speichert unter einem anderen Namen. `*` im Fenstertitel kennzeichnet ungespeicherte Änderungen. |
| Einfügen | **Leere Seite…** oder **Aus PDF oder DOCX einfügen…** wählen. Positionen beginnen bei 1; Seitenzahl plus 1 fügt am Dokumentende ein. Eine leere Seite übernimmt die Größe der angrenzenden Seite. Eingefügt wird die gesamte ausgewählte PDF oder jede aus der DOCX erzeugte Seite. Für DOCX gelten dieselbe native Import-Engine und dieselben Layout-Einschränkungen wie unten beschrieben. Das Word-Original bleibt unverändert; Abbruch oder Fehler erhalten das aktuelle Dokument. |
| Zoom / Anpassung | Reiter oder Menü Ansicht bzw. unteres Zoomfeld verwenden. `Ctrl+0`: tatsächliche Größe; `Ctrl+1`: ganze Seite; `Ctrl+2`: Seitenbreite. |

**Bearbeiten → Text hinzufügen…** (`Ctrl+Shift+T`) wählen und auf die Seite klicken. Mehrzeiligen Text eingeben, Schriftart, Größe und Farbe auswählen und bestätigen. Esc bricht die Platzierung ab. Eingefügter Text bleibt in der gespeicherten PDF durchsuchbar; vorhandener Text wird nicht verändert. Text über dem verbleibenden Seitenbereich wird abgelehnt.

Hinzugefügte Texte und Unterschriften sind bearbeitbare Objekte. Zum Auswählen anklicken oder **Bearbeiten → Text oder Unterschrift auswählen** verwenden. Beim Ziehen bewegen sich Inhalt und Auswahlrahmen gemeinsam in Echtzeit. Eckgriffe skalieren proportional; Text behält Zeilenumbrüche und Durchsuchbarkeit. Loslassen übernimmt die Änderung, Esc während des Ziehens verwirft die Vorschau. Pfeiltasten verschieben um 1 PDF-Punkt (Umschalt: 10 Punkte). Entf/Rücktaste oder **Ausgewähltes Objekt löschen** entfernt das Objekt. Esc hebt die Auswahl auf. Objekte bleiben innerhalb der Seite. Doppelklick oder F2/Enter nach der Auswahl öffnet den Editor. Textinhalt, Schriftart, Größe und Farbe sind änderbar; die Schriftgröße berücksichtigt die Skalierung. Unterschriften lassen sich ergänzen, löschen/neuzeichnen oder durch ein Bild ersetzen. Position und Größe der Unterschrift bleiben erhalten. Text behält Position und Breite; die Höhe passt sich dem Inhalt an. Abbrechen erhält das Objekt; Änderungen über den Seitenrand werden abgelehnt. MyPdf speichert ursprüngliche Seiten und Objektdaten als eingebetteten Anhang zusätzlich zur normalen PDF-Darstellung. Beim erneuten Öffnen bleiben Auswahl und Bearbeitung möglich, auch nach Umsortieren, Einfügen oder Extrahieren. Andere PDF-Betrachter zeigen die zusammengesetzten Seiten normal an. Die Dateigröße steigt dadurch. Bereits in älteren oder externen PDFs fest eingebettete Texte und Unterschriften sind damit nicht auswählbar.

## DOCX-Import zur PDF-Bearbeitung

Öffnen Sie `.docx` über **Datei → Öffnen…**, den Startbildschirm oder ein Dateipfadargument. MyPdf liest ZIP-Paket und WordprocessingML selbst, rendert die unterstützten Inhalte in eine private temporäre PDF und öffnet sofort die PDF-Bearbeitung. Word, LibreOffice, unzip-Prozesse oder externe Konverter werden nicht verwendet. Das alte `.doc`-Format wird nicht unterstützt.

Die importierte PDF beginnt **ungespeichert** mit einem vorgeschlagenen `.pdf`-Namen auf Basis des Word-Dateinamens. Vorschauen, Zoom, Unterschriften, Verschieben, Einfügen und Extrahieren stehen sofort zur Verfügung. **Speichern** (`Ctrl+S`) fragt beim ersten Mal nach einer PDF-Zieldatei; danach wird diese PDF aktualisiert. **Speichern unter…** (`Ctrl+Shift+S`) wählt einen anderen PDF-Pfad. Beim Schließen oder Ersetzen eines ungespeicherten Imports erscheint eine Speicherabfrage. Das Word-Original bleibt erhalten; sein Pfad ist gegen Überschreiben geschützt. Ein fehlgeschlagener Import erhält das aktuelle Dokument.

Der native Leser unterstützt Absätze, Überschriften und einfache vererbte Formatvorlagen, Fett/Kursiv/Unterstreichen/Durchstreichen, Schriftgröße und Farbe, Ausrichtung, einfache Aufzählungen und Dezimallisten, Tabellen mit horizontal verbundenen Zellen sowie eingebettete PNG/JPEG/BMP/GIF/WebP-Bilder. Die PDF wird als A4 mit 20 mm Rand und automatischen Seitenumbrüchen erzeugt; Text bleibt durchsuchbar. Externe Bilder und beliebige Dokumentressourcen werden nicht geladen.

Der Import bildet die Word-Seitenlayout-Engine nicht nach. Schriften und Layout können abweichen. DOCX-Bearbeitung und Speichern im DOCX-Format werden nicht unterstützt. Kopf-/Fußzeilen, Fuß-/Endnoten, Kommentare, Änderungsverfolgung, komplexe Nummerierungen, vertikal verbundene Zellen, schwebende Bildpositionen, Diagramme, Formeln und Zeichenobjekte werden nicht vollständig dargestellt. Gelöschter Änderungstext wird ausgelassen, eingefügter Text angezeigt. Makros werden nicht ausgeführt. Passwortverschlüsselte Pakete und ZIP64 werden nicht unterstützt. Grenzen: 64 MiB Paketgröße, 16 MiB pro ZIP-Eintrag, 128 MiB deklarierte entpackte Gesamtgröße sowie Beschränkungen für XML-Tiefe/-Knoten, Bilder und Tabellen. Nicht unterstützte Bilder erscheinen als Platzhalter.

## Speichern und Einschränkungen

Die Bearbeitung erfolgt in privaten temporären Kopien. Die Quelldatei ändert sich erst beim Speichern; `QSaveFile` ersetzt die Zieldatei atomar. Fehlgeschlagene Bearbeitungen lassen das aktuelle Dokument unverändert. Temporäre Dateien werden beim regulären Beenden gelöscht. Seitenoperationen erhalten den ursprünglichen Text und die Vektorinhalte, statt die gesamte PDF zu rastern.

Zertifikatsbasierte Signaturen und Identitätsprüfung sind nicht implementiert. Die Bearbeitung einer PDF mit vorhandener digitaler Signatur kann diese ungültig machen.

Passwortgesperrte PDFs, allgemeines Rückgängigmachen/Wiederholen, direkte Textbearbeitung und OCR werden nicht unterstützt. Prüfen Sie nach dem Extrahieren oder Umsortieren dokumentweite Lesezeichen, interne Verknüpfungen und Formulare gesondert. Darstellung und PDF-Verarbeitung laufen derzeit im UI-Thread; große Dokumente können die Oberfläche vorübergehend blockieren.

## Projektstruktur und Tests

| Pfad | Zweck |
| --- | --- |
| `src/document.*` | PDF-Laden, Darstellung, Seitenoperationen, Einfügen und Abbrechen von Unterschriften sowie atomares Speichern. |
| `src/main.cpp` | Qt-Widgets-Oberfläche, Dialoge, Unterschriftsplatzierung und Spracheinstellungen. |
| `src/icons.*` | Einfarbige Vektorsymbole, Anwendungsstil und Symbolanbieter für Dateidialoge. |
| `src/i18n.*` | Englische Ausgangstexte, Übersetzungsabfrage und Qt-Dialogübersetzungen. |
| `translations/` | Deutsche und koreanische JSON-Kataloge mit englischen Textschlüsseln, eingebettet als Qt-Ressourcen. |
| `tests/document_test.cpp` | PDF-Operationen, durchsuchbarer Text, Erhalt der Originaldatei, Fehlerbehandlung, Drehung und beschnittene Seiten. |
| `tests/localization_test.cpp` | Sprachwechsel, gespeicherte Einstellungen, übersetzte Meldungen und Erhalt des Bearbeitungszustands. |
| `tests/objects_test.cpp` | Bearbeitbare Objekte: Speichern/Öffnen, Verschieben/Löschen, Seitenoperationen, Maus/Tastatur und Fehlerbehandlung. |
| `tests/signature_test.cpp` | Abbrechen vor, während und nach der Platzierung sowie Grenzen beim Speichern und bei späteren Bearbeitungen. |
| `src/toolpaths.*` | Plattformübergreifende qpdf-Suche über Vorgabe, Programmverzeichnis, macOS-Helper, PATH und Konfigurationspfad. |
| `tests/toolpaths_test.cpp` | Suchreihenfolge und Pfade mit Leerzeichen. |
| `cmake/`, `packaging/`, `.github/workflows/` | Build-Konfiguration, native App-Ressourcen und CI für drei Plattformen. |
| `src/docx.*` | Größenbegrenzter ZIP-/WordprocessingML-Leser mit Qt-Darstellung, ohne externe Konvertierung. |
| `tests/docx_test.cpp` | DOCX-Formatierung, Listen, Tabellen, Bilder, ungültige Eingaben, PDF-Export/Seitenumbrüche und PDF-Import, Bearbeitung und Speichern. |
| `scripts/` | Hilfsskripte zum Kompilieren, Testen und Starten. |
| `.vscode/` | CMake- und IntelliSense-Konfiguration mit `build/compile_commands.json`. |
| `build/` | Generierte Programme, Compiler-Ausgaben und CMake-Dateien; werden beim Kompilieren neu erstellt. |
| `output/` | Optionale erzeugte Dateien, etwa die aus einer früheren README konvertierte Test-PDF. |

Mit `./scripts/build.sh` kompilieren und testen Sie das Projekt. `ctest --test-dir build --output-on-failure` führt die bereits kompilierten Tests aus. Die automatisierten GUI-Tests verwenden die Offscreen-Plattform von Qt.

## Dokumentationspflege

`README.md` ist die englische Referenz. `README.de.md` und `README.ko.md` enthalten dieselben Anleitungen auf Deutsch und Koreanisch. Bei Änderungen an Verhalten, Abhängigkeiten oder Befehlen aktualisieren Sie alle drei Dokumente gemeinsam. Halten Sie Abschnitte und Beispiele aufeinander abgestimmt; die UI-Übersetzungen werden separat unter `translations/` gepflegt.

Prüfen Sie vor der Weitergabe die Lizenzen von Qt 6, Poppler und qpdf. Eine projektweite Lizenzdatei ist derzeit nicht enthalten.
