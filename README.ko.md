# MyPdf

[English](README.md) | [Deutsch](README.de.md) | **한국어**

Ubuntu·Windows·macOS를 대상으로 하는 C++17 / Qt 6 PDF 편집기입니다. 문서를 읽고, 페이지를 정리하고, 손글씨·이미지 서명을 추가할 수 있습니다. PDF 표시는 Poppler, 페이지 편집은 qpdf를 사용합니다.

## 주요 기능

- PDF 파일 열기·닫기
- DOCX를 편집 가능한 PDF로 가져오기. 구형 DOC는 미지원
- 서명 그리기·서명 이미지 가져오기
- 선택 페이지를 별도 PDF로 추출하여 문서 분할
- 문서 내 페이지 순서 이동
- 저장·다른 이름으로 저장
- 빈 페이지·다른 PDF의 전체 페이지·DOCX에서 생성한 전체 페이지 삽입
- 재시작 없이 영어·독일어·한국어 UI 전환
- 밝은 테마와 어두운 테마 전환 및 선택 저장

서명은 **시각적 서명**이며, 인증서 기반 디지털 서명은 아닙니다.

앱 아이콘은 선택한 08번 포메라니안이며 `assets/icons/MyPdf.png`에 저장하고 실행 파일에 포함합니다. Linux 설치 시 `packaging/MyPdf.desktop`과 아이콘도 설치합니다. 프로젝트 폴더에서 `cmake --install build --prefix "$HOME/.local"`을 실행하면 현재 사용자용으로 설치됩니다. 설치된 데스크톱 항목은 실행 파일과 아이콘의 절대 경로를 사용하므로 앱 메뉴 실행은 `PATH`에 의존하지 않습니다. hicolor 아이콘도 설치하고 사용 가능한 데스크톱·아이콘 캐시를 갱신합니다.

Linux 개발 중에는 빌드 후 `./scripts/install-launcher.sh`를 실행하면 실행 파일을 복사하지 않고 현재 빌드와 아이콘을 사용자 앱 메뉴에 등록합니다. MyPdf를 종료한 뒤 앱 메뉴에서 다시 실행하세요. 독에 이전 즐겨찾기가 남아 있으면 제거 후 다시 고정하세요. 프로젝트 위치를 옮겼다면 스크립트를 다시 실행합니다. 등록 위치는 `${XDG_DATA_HOME:-$HOME/.local/share}`입니다.

## 빌드 및 실행

CMake 3.22 이상, C++17 컴파일러, Qt 6.4 이상 Widgets, Poppler의 Qt 6 바인딩, qpdf, zlib가 필요합니다. 각 운영체제에서 아키텍처와 컴파일러가 일치하는 의존성으로 직접 빌드합니다. 다른 운영체제에서 사용한 `build/` 폴더를 재사용하거나 기존 폴더의 CMake 생성기를 바꾸지 마세요.

| 운영체제 | 구성 | 검증 상태 |
| --- | --- | --- |
| Ubuntu 26.04 | GCC, 시스템 Qt / Poppler / qpdf | 이번 수정 후 로컬 빌드와 테스트 통과. |
| Windows x64 | MSYS2 UCRT64, GCC, Ninja | 빌드 구성과 네이티브 CI 추가. 이번 세션에서 Windows 실행 검증은 하지 않음. |
| macOS | Apple Clang, Homebrew, 현재 기기의 아키텍처 | 빌드 구성과 네이티브 CI 추가. 이번 세션에서 macOS 실행 검증은 하지 않음. |

### Ubuntu

프로젝트 폴더에서 실행합니다.

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config qt6-base-dev libpoppler-qt6-dev qpdf qt6-translations-l10n zlib1g-dev
./scripts/build.sh
./scripts/run.sh
```

### Windows: MSYS2 UCRT64

[MSYS2](https://www.msys2.org/)를 설치하고 **UCRT64** 터미널을 연 뒤 프로젝트 폴더로 이동합니다. 예: `cd /c/dev/MyPdf`. 아래 명령을 실행하세요. 첫 업데이트에서 터미널 종료를 요청하면 UCRT64를 다시 열어 업데이트를 마친 뒤 패키지를 설치합니다.

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-translations mingw-w64-ucrt-x86_64-poppler-qt6 mingw-w64-ucrt-x86_64-qpdf mingw-w64-ucrt-x86_64-zlib
./scripts/build.sh
./scripts/run.sh
```

결과물은 포메라니안 `.ico` 아이콘을 포함한 `build/MyPdf.exe`입니다. Qt·Poppler·컴파일러 DLL을 `PATH`에서 찾도록 UCRT64에서 실행하세요. 문서에서 지원하는 구성은 MinGW 방식이며, MSVC용 Qt와 라이브러리를 섞지 마세요. 패키지 정보: [UCRT64용 Poppler Qt 6](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-poppler-qt6).

### macOS: Homebrew

먼저 [Homebrew](https://brew.sh/)를 설치합니다. 프로젝트 폴더에서 다음을 실행하세요. Command Line Tools가 이미 있으면 `xcode-select --install`은 생략합니다.

```bash
xcode-select --install
brew install cmake ninja pkgconf qtbase qttranslations poppler-qt6 qpdf zlib
./scripts/build.sh
./scripts/run.sh
```

`build.sh`가 Homebrew의 Qt와 Poppler 경로를 자동 지정합니다. Qt 6 바인딩이 필요하므로 `poppler`만 설치하지 말고 **poppler-qt6**를 사용하세요. 이 패키지는 keg-only입니다. [Homebrew 패키지 설명](https://formulae.brew.sh/formula/poppler-qt6)을 참고하세요. 결과물은 `build/MyPdf.app`이며 실제 실행 파일은 `build/MyPdf.app/Contents/MacOS/MyPdf`입니다. 포메라니안 `.icns` 아이콘을 포함합니다. Apple Silicon·Intel은 각각 현재 기기에 맞는 의존성을 사용하며, universal binary를 생성하지는 않습니다.

### 공통 명령과 설치

`build.sh`는 Debug 빌드 후 테스트를 실행하고, `run.sh`는 운영체제에 맞는 실행 파일을 선택합니다. Release 빌드는 아래 명령을 사용하되 Windows에서는 configure 명령에 `-G Ninja -DCMAKE_PREFIX_PATH="$MINGW_PREFIX"`, macOS에서는 `-DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)"`를 추가해 같은 의존성 경로를 지정합니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./scripts/run.sh '/path/to/document.pdf'
```

개발용 설치는 `cmake --install build --prefix /path/to/install`로 수행합니다. macOS는 `MyPdf.app`, Windows는 `bin/MyPdf.exe`, Linux는 실행 파일과 데스크톱 런처·아이콘을 설치합니다. **의존성을 모두 포함한 독립 배포 패키지는 아닙니다.** Qt 플러그인·라이브러리, Poppler 의존성, qpdf와 그 의존성이 실행 환경에 있어야 합니다. 자동 DLL·dylib 배포, 설치 프로그램, macOS 서명·공증은 구현하지 않았습니다.

qpdf 탐색 순서는 `MYPDF_QPDF`, 앱 실행 파일 옆, `../Helpers/qpdf` 또는 `qpdf.exe`, `PATH`, CMake 구성 시 발견한 경로입니다. 마지막 경로는 빌드한 컴퓨터에 종속된 개발용 대체 경로입니다. Finder에서 실행하려면 지원 경로에 qpdf를 두거나 설정된 터미널에서 `run.sh`를 사용하세요. 탐색을 직접 지정하려면 `MYPDF_QPDF`에 추가 인자 없이 실행 파일 경로만 넣습니다.

[`.github/workflows/build.yml`](.github/workflows/build.yml)은 GitHub에 올렸을 때 Ubuntu·Windows UCRT64·macOS에서 빌드와 테스트를 실행합니다. 워크플로 파일을 추가한 것과 실제 원격 작업이 통과한 것은 구분해야 합니다.

## 화면 구성

- **메뉴 바:** 파일·편집·보기·페이지·설정·도움말. 텍스트와 서명은 편집 메뉴에, 페이지 편집은 페이지 메뉴에 모았습니다.
- **기본 도구 모음:** 열기·저장·파일명과 선택·텍스트·서명·페이지를 한 줄에 배치했습니다. 서명 버튼은 바로 그리기, 옆 화살표는 이미지 가져오기입니다. 페이지 버튼에서 이동·삽입·추출을 선택합니다. 다른 이름으로 저장과 문서 닫기는 파일 메뉴에 있습니다.
- **상황별 도구:** 개체를 선택할 때만 수정·삭제 버튼이 나타납니다. 개체 우클릭으로도 수정·삭제가 가능하고 더블클릭하면 바로 수정합니다. 배치 중에만 취소 버튼을 보여주며, 작업 탭은 제거했습니다.
- **왼쪽 패널:** 페이지 미리보기, 다중 선택, 우클릭 페이지 구성 메뉴. 보기 → 페이지 미리보기에서 표시 여부를 바꿉니다.
- **하단 탐색:** 이전·다음 페이지, 페이지 번호, 축소·확대, 40~300% 배율 입력과 맞춤 메뉴(페이지·너비·실제 크기)를 제공합니다. 문서는 처음 열 때 페이지에 맞춥니다.

파일 대화상자와 메시지 창까지 동일한 단색 선형 아이콘을 사용합니다.

## 표시 언어 설정

**Settings → Language → English / Deutsch / 한국어**에서 선택합니다. 독일어 메뉴 경로는 **Einstellungen → Sprache**, 한국어는 **설정 → 언어**입니다.

메뉴, 도구 모음, 페이지 이름, 프로그램 안내·오류 메시지가 즉시 바뀝니다. 문서, 저장하지 않은 편집, 선택한 페이지, 확대 비율과 현재 도구 상태는 유지됩니다.

기본 언어와 번역 누락 시의 대체 언어는 영어입니다. 선택값은 Qt `QSettings`의 `ui/language`에 저장되며, 기존 설정이 있으면 우선 적용됩니다. Ubuntu 기본 경로는 `~/.config/MyPdf/MyPdf.conf`이며 `XDG_CONFIG_HOME` 설정을 따릅니다.

Qt 기본 대화상자 번역에는 위에서 안내한 운영체제별 번역 패키지가 필요합니다(Ubuntu: `qt6-translations-l10n`). 없으면 일부 기본 문구가 영어로 표시될 수 있습니다. 파일 선택창은 앱에서 선택한 언어를 적용하도록 Qt 대화상자를 사용합니다.

## 화면 테마

**설정 → 화면 테마 → 밝게 / 어둡게**에서 즉시 전환할 수 있습니다. 선택한 테마는 Qt `QSettings`의 `ui/theme`에 저장되며, 기본값은 밝은 테마입니다. PDF 페이지는 문서의 원래 색상을 유지합니다.

## 사용법

아래 단축키 표기는 Ubuntu·Windows 기준입니다. macOS에서는 해당 Qt 표준 단축키가 Command 키로 표시됩니다.

| 작업 | 조작 방법 |
| --- | --- |
| 열기·닫기 | 파일 메뉴 또는 `Ctrl+O` / `Ctrl+W`. 저장하지 않은 변경이 있으면 저장 여부를 묻습니다. |
| 서명 추가 | **서명 그리기…** 또는 **서명 이미지 가져오기…**를 선택한 뒤 페이지에서 사각형을 드래그해 위치와 크기를 지정합니다. 투명 PNG는 배경을 유지합니다. 이미지가 사각형에 맞춰 늘어나므로 서명 비율에 맞게 드래그하세요. |
| 서명 취소 | **서명 배치 취소** 또는 `Esc`로 배치 대기·드래그 중인 서명을 취소합니다. 배치한 뒤에는 서명을 선택해 삭제하거나 이동합니다. |
| 분할·추출 | `Ctrl` / `Shift`로 미리보기를 선택한 뒤 **선택 페이지 추출…**을 실행합니다. 선택 페이지를 문서 순서대로 새 PDF에 저장하며 현재 문서의 페이지는 유지합니다. 서로 다른 묶음을 각각 추출해 문서를 나눕니다. |
| 순서 이동 | **앞으로 이동** / **뒤로 이동** 또는 `Alt+↑` / `Alt+↓`. 여러 페이지를 선택해도 현재 활성 페이지 하나만 이동합니다. |
| 저장 | `Ctrl+S`는 현재 경로, `Ctrl+Shift+S`는 다른 이름으로 저장합니다. 창 제목의 `*`는 저장하지 않은 변경을 뜻합니다. |
| 삽입 | **빈 페이지…** 또는 **PDF 또는 DOCX에서 삽입…**을 선택합니다. 위치는 1부터 시작하며 전체 페이지 수 + 1은 문서 끝입니다. 빈 페이지는 인접 페이지 크기를 사용하고, 문서 삽입은 선택한 PDF 전체 또는 DOCX에서 생성한 모든 페이지를 삽입합니다. DOCX는 아래의 자체 가져오기 엔진과 배치 제한을 동일하게 적용합니다. Word 원본은 유지하며 취소하거나 삽입에 실패하면 현재 문서를 유지합니다. |
| 확대·맞춤 | 보기 탭·메뉴 또는 하단 확대 비율을 사용합니다. `Ctrl+0`: 실제 크기, `Ctrl+1`: 페이지 맞춤, `Ctrl+2`: 너비 맞춤. |

**편집 → 텍스트 추가…** (`Ctrl+Shift+T`)를 선택한 뒤 페이지의 삽입 위치를 클릭합니다. 여러 줄 텍스트를 입력하고 글꼴·크기·색상을 선택해 확인합니다. Esc로 배치를 취소합니다. 저장된 PDF에서도 텍스트 검색이 가능하며 기존 글자를 수정하는 기능은 아닙니다. 페이지 높이를 벗어나는 입력은 삽입하지 않습니다.

추가한 텍스트와 서명은 편집 가능한 개체입니다. 개체를 클릭해 선택하거나 **편집 → 텍스트 또는 서명 선택**을 사용합니다. 드래그하면 내용과 선택 테두리가 실시간으로 함께 이동합니다. 모서리 핸들을 드래그하면 비율을 유지하며 크기가 바뀌고, 텍스트의 줄바꿈과 검색 기능도 유지됩니다. 마우스를 놓으면 반영하며 드래그 중 Esc를 누르면 변경을 취소합니다. 방향키는 PDF 기준 1pt, Shift+방향키는 10pt씩 이동합니다. Delete/Backspace 또는 **선택한 개체 삭제**로 삭제하고 Esc로 선택을 해제합니다. 개체는 페이지 안에서만 이동합니다. 개체를 더블클릭하거나 선택 후 F2/Enter를 누르면 다시 수정할 수 있습니다. 텍스트는 내용·글꼴·크기·색상을 바꾸며, 크기 조절 후의 실제 표시 글자 크기를 불러옵니다. 서명은 기존 그림에 덧그리기·지우고 다시 그리기·이미지 교체가 가능합니다. 서명 수정 시 위치와 크기를 유지하고, 텍스트는 위치와 너비를 유지하며 내용에 맞게 높이를 조정합니다. 취소하면 기존 개체를 유지하며 페이지를 벗어나는 변경은 반영하지 않습니다. MyPdf는 일반 PDF 표시 내용과 함께 원본 페이지·개체 데이터를 첨부파일로 저장하므로, 저장 후 다시 열어도 선택·이동·삭제할 수 있습니다. 페이지 재배치·삽입·추출 시에도 편집 정보를 유지합니다. 다른 PDF 뷰어에서는 합성된 페이지가 정상 표시됩니다. 편집 정보로 인해 파일 크기가 증가할 수 있습니다. 이전 버전이나 다른 프로그램에서 이미 페이지에 합쳐진 텍스트·서명은 이 기능으로 선택할 수 없습니다.

## DOCX 가져오기 및 PDF 편집

**파일 → 열기…**, 시작 화면 또는 실행 인자로 `.docx`를 열 수 있습니다. MyPdf가 ZIP 패키지와 WordprocessingML을 직접 읽고, 지원하는 내용을 전용 임시 PDF로 만든 뒤 즉시 PDF 편집 화면을 엽니다. Word·LibreOffice·unzip 프로세스나 외부 변환 도구를 사용하지 않습니다. 구형 `.doc`는 지원하지 않습니다.

가져온 PDF는 **저장되지 않은 상태**로 시작하며 Word 파일명을 바탕으로 `.pdf` 이름을 제안합니다. 미리보기·확대·서명·페이지 이동·삽입·추출 기능을 바로 사용할 수 있습니다. 첫 **저장** (`Ctrl+S`)에서는 PDF 저장 경로를 선택하고, 이후 저장은 해당 PDF를 갱신합니다. **다른 이름으로 저장…** (`Ctrl+Shift+S`)은 다른 PDF 경로를 선택합니다. 저장 전 닫거나 다른 문서를 열면 저장 여부를 묻습니다. Word 원본은 유지하며 원본 경로에 덮어쓰는 것도 방지합니다. 가져오기에 실패하면 현재 문서를 유지합니다.

자체 리더는 본문·제목·기본 스타일 상속, 굵게·기울임·밑줄·취소선, 글자 크기·색, 정렬, 단순 글머리표·십진 번호 목록, 가로 셀 병합이 있는 기본 표, 포함된 PNG/JPEG/BMP/GIF/WebP 이미지를 지원합니다. PDF는 A4·20 mm 여백으로 자동 페이지를 나누며 텍스트 검색이 가능합니다. 외부 이미지나 임의 문서 리소스는 가져오지 않습니다.

가져오기는 Microsoft Word와 동일한 페이지 배치 엔진을 사용하지 않습니다. 글꼴과 배치는 달라질 수 있습니다. DOCX 자체 편집이나 DOCX 형식으로의 저장은 지원하지 않습니다. 머리말·꼬리말, 각주·미주, 댓글, 변경 추적 검토, 복잡한 번호 목록, 세로 셀 병합, 부동 이미지 위치, 차트·수식·그리기 개체는 완전히 표시하지 못합니다. 변경 추적의 삭제 텍스트는 생략하고 삽입 텍스트는 표시합니다. 매크로는 실행하지 않습니다. 암호화된 패키지와 ZIP64는 지원하지 않습니다. 제한은 패키지 64 MiB, ZIP 항목당 16 MiB, 선언된 전체 압축 해제 크기 128 MiB이며 XML 깊이·노드 수와 이미지·표 크기도 제한합니다. 지원하지 않는 이미지는 자리표시자로 표시합니다.

## 저장 방식과 제한 사항

편집은 전용 임시 사본에서 수행합니다. 원본은 저장할 때만 변경되며 `QSaveFile`로 저장 대상을 원자적으로 교체합니다. 편집에 실패하면 현재 문서를 유지합니다. 임시 파일은 정상 종료 시 삭제됩니다. 페이지 편집 시 PDF 전체를 이미지로 바꾸지 않고 기존 텍스트와 벡터 내용을 유지합니다.

인증서 기반 서명과 본인 인증은 구현되어 있지 않습니다. 기존 디지털 서명이 있는 PDF를 편집하면 그 서명의 유효성이 깨질 수 있습니다.

암호로 잠긴 PDF, 일반적인 실행 취소·다시 실행, 텍스트 직접 편집, OCR은 지원하지 않습니다. 페이지 추출·재배치 후에는 문서 수준의 책갈피, 내부 링크, 양식을 별도로 확인하세요. 렌더링과 PDF 처리는 현재 UI 스레드에서 실행하므로 큰 문서를 처리하는 동안 화면이 일시적으로 응답하지 않을 수 있습니다.

## 프로젝트 구성 및 테스트

| 경로 | 역할 |
| --- | --- |
| `src/document.*` | PDF 열기·렌더링·페이지 편집·서명 합성 및 취소·원자적 저장. |
| `src/main.cpp` | Qt Widgets 화면, 대화상자, 서명 배치, 언어 설정. |
| `src/icons.*` | 단색 벡터 아이콘, 앱 스타일, 파일 대화상자 아이콘 제공자. |
| `src/i18n.*` | 영어 기준 문구, 번역 조회, Qt 기본 대화상자 번역. |
| `translations/` | 영어 문구를 키로 사용하는 독일어·한국어 JSON 번역. Qt 리소스로 포함됩니다. |
| `tests/document_test.cpp` | PDF 편집, 텍스트 검색 가능 여부, 원본 보존, 오류 처리, 회전·CropBox 페이지 검증. |
| `tests/localization_test.cpp` | 언어 전환, 설정 복원, 메시지 번역, 편집 상태 유지 검증. |
| `tests/objects_test.cpp` | 편집 개체 저장·다시 열기·이동·삭제·페이지 편집·마우스/키보드·실패 시 상태 보존. |
| `tests/signature_test.cpp` | 서명 배치 취소·개체 선택/삭제·텍스트 입력 대화상자 검증. |
| `src/toolpaths.*` | 환경 변수·실행 파일 옆·macOS Helpers·PATH·구성 시 경로를 통한 qpdf 탐색. |
| `tests/toolpaths_test.cpp` | 도구 탐색 우선순위와 공백이 포함된 경로 검증. |
| `cmake/`, `packaging/`, `.github/workflows/` | 빌드 설정 생성, 운영체제별 앱 리소스, 세 운영체제 CI. |
| `src/docx.*` | 크기 제한을 둔 ZIP·WordprocessingML 직접 읽기와 Qt 표시. 외부 변환 없음. |
| `tests/docx_test.cpp` | DOCX 서식·목록·표·이미지·잘못된 입력·PDF 내보내기 및 페이지 나눔·PDF 가져오기·편집·저장 흐름 검증. |
| `scripts/` | 빌드·테스트·실행 도우미. |
| `.vscode/` | `build/compile_commands.json`을 사용하는 CMake·IntelliSense 설정. |
| `build/` | 실행 파일, 컴파일 결과, CMake 파일 등 다시 생성할 수 있는 빌드 산출물. |
| `output/` | 이전 README에서 변환한 테스트 PDF 등 선택적으로 생성하는 산출물. |

`./scripts/build.sh`로 빌드와 테스트를 실행합니다. 기존 테스트 빌드만 실행하려면 `ctest --test-dir build --output-on-failure`를 사용합니다. 자동 GUI 테스트는 Qt의 offscreen 플랫폼을 사용합니다.

## 문서 관리

`README.md`를 영어 기준 문서로 관리합니다. `README.de.md`와 `README.ko.md`는 동일한 안내의 독일어·한국어 버전입니다. 기능, 의존 패키지, 명령어가 바뀌면 세 문서를 같은 변경에서 함께 수정합니다. 목차와 예제의 구성을 맞추며, UI 번역은 `translations/`에서 별도로 관리합니다.

프로그램 배포 전 Qt 6, Poppler, qpdf의 라이선스를 확인하세요. 현재 프로젝트 전체에 적용되는 라이선스 파일은 포함되어 있지 않습니다.
