# MyPdf

[English](README.md) | [Deutsch](README.de.md) | **한국어**

Ubuntu·Windows·macOS용 C++17 / Qt 6 PDF 편집기입니다. PDF를 열고 페이지를 정리하며 텍스트와 서명을 추가할 수 있습니다.

## 주요 기능

- PDF 열기·닫기·저장·다른 이름으로 저장
- 페이지 이동, 선택한 페이지를 새 PDF로 추출, 빈 페이지·PDF·DOCX 삽입
- 텍스트 추가, 서명 그리기, 서명 이미지 가져오기
- 형광펜으로 강조 표시: 색상·굵기·불투명도 조절, Shift로 수평선 그리기
- 추가한 텍스트·서명의 선택·드래그 이동·크기 조절·더블클릭 수정·삭제
- DOCX를 PDF로 가져와 편집·저장: Word 또는 LibreOffice 설치 불필요
- 페이지 미리보기, 상단 도구 모음의 확대·축소 및 페이지·너비 맞춤
- 영어·독일어·한국어 전환과 밝은·어두운 테마

## 공통 준비 사항

| 구분 | 필요한 항목 |
| --- | --- |
| 빌드 도구 | C++17 컴파일러, CMake 3.22 이상, Make 또는 Ninja, Bash |
| 라이브러리 | Qt 6.4 이상 Widgets, Poppler **Qt 6 바인딩**, zlib |
| 실행 도구 | qpdf: 페이지 편집·텍스트·서명 합성에 필요 |
| 언어 리소스 | Qt 번역 패키지: 기본 파일 대화상자 등의 번역 |
| Linux 패키징 | Python 3, binutils; 최초 도구 다운로드용 인터넷 연결 |
| macOS 패키징 | Python 3, Xcode Command Line Tools, 빌드에 사용한 Qt의 macdeployqt |

소스를 Git으로 받거나 ZIP으로 내려받아 압축을 푼 뒤, **프로젝트 최상위 폴더**에서 명령을 실행하세요. 컴퓨터마다 OS에 맞는 의존성을 설치하고 새로 빌드합니다. `build/`, CMake 캐시와 다른 OS의 실행 파일은 재사용하지 않습니다. Qt·Poppler·컴파일러는 같은 아키텍처와 도구 모음으로 맞추세요.

## Ubuntu 설치·빌드

Ubuntu 26.04 기준입니다. GCC와 시스템 패키지를 사용합니다.

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config qt6-base-dev libpoppler-qt6-dev qpdf qt6-translations-l10n zlib1g-dev
./scripts/build.sh
./scripts/run.sh
```

결과물은 `build/MyPdf`입니다. 사용자 앱 메뉴와 아이콘을 포함해 설치하려면 다음을 실행합니다. 설치 후에는 앱 메뉴에서도 실행할 수 있습니다.

```bash
cmake --install build --prefix "$HOME/.local"
"$HOME/.local/bin/MyPdf"
```

이 설치는 시스템의 Qt·Poppler·qpdf를 계속 사용합니다. 업데이트는 앱을 종료하고 다시 빌드한 뒤 설치 명령을 반복합니다.

## Windows 설치·빌드

1. [MSYS2](https://www.msys2.org/)를 설치합니다.
2. **MSYS2 UCRT64** 터미널을 열고 업데이트합니다. 터미널을 닫으라는 안내가 있으면 UCRT64를 다시 열어 업데이트를 완료하세요.

```bash
pacman -Syu
```

3. 프로젝트 폴더로 이동합니다. 예: `cd /c/dev/MyPdf`. 같은 UCRT64 터미널에서 의존성을 설치하고 빌드합니다.

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

결과물은 `build/MyPdf.exe`입니다. 스크립트가 Ninja와 UCRT64 라이브러리 경로를 설정합니다. **MSYS·MINGW64·MSVC용 Qt와 혼용하지 마세요.** 실행 시에도 UCRT64 터미널을 사용해야 `/ucrt64/bin`의 DLL과 qpdf를 찾을 수 있습니다. [MSYS2 환경 설명](https://www.msys2.org/docs/environments/)

별도 위치에 설치하려면 같은 터미널에서 실행합니다.

```bash
cmake --install build --prefix "$HOME/Apps/MyPdf"
"$HOME/Apps/MyPdf/bin/MyPdf.exe"
```

이 설치는 DLL을 포함하는 독립 패키지가 아닙니다.

## macOS 설치·빌드

1. Apple 개발 도구를 설치하고 설치 창의 작업을 완료합니다. 이미 설치되어 있다면 생략하세요.

```bash
xcode-select --install
```

2. [Homebrew](https://brew.sh/)를 설치하고 설치 마지막에 안내되는 `brew shellenv` 설정을 셸에 적용합니다. 터미널에서 `brew --version`이 실행되어야 합니다. Apple Silicon은 네이티브 ARM 환경, Intel은 x86_64 환경을 사용하며 Rosetta용 의존성과 섞지 않습니다.
3. 프로젝트 폴더에서 의존성을 설치하고 빌드합니다.

```bash
brew install cmake pkgconf qtbase qttranslations poppler-qt6 qpdf zlib python
./scripts/build.sh
./scripts/run.sh
```

`poppler-qt6`는 일반 `poppler`와 별도 패키지입니다. 스크립트가 Homebrew의 Qt·Poppler·zlib 및 qpdf 경로를 자동 설정합니다. [Homebrew 패키지 정보](https://formulae.brew.sh/formula/poppler-qt6)

`vulkan-headers`는 Qt의 선택적 헤더 탐색용이며 MyPdf 실행 의존성이 아닙니다. 필요할 때만 설치합니다. Vulkan SDK·런타임은 필요하지 않습니다.

```bash
brew install vulkan-headers
```

결과물은 `build/MyPdf.app`입니다. 기본 GUI 테스트는 Cocoa를 사용하므로 로그인된 macOS 데스크톱 세션에서 실행하세요. 일반 앱처럼 로컬에 설치하려면 다음을 실행합니다.

```bash
mkdir -p "$HOME/Applications"
cmake --install build --prefix "$HOME/Applications"
open "$HOME/Applications/MyPdf.app"
```

Finder에서 앱을 더블클릭하거나 Dock에 고정해 실행할 수 있으며 `run.sh`는 필요하지 않습니다. 이 설치는 Homebrew 의존성을 유지해야 합니다. 앱을 종료하고 빌드·설치 명령을 반복하면 업데이트됩니다.

## Release 빌드

각 OS에서 위 `./scripts/build.sh`를 한 번 완료한 뒤 실행합니다. 기존 CMake 설정에 저장된 OS별 의존성 경로와 생성기를 사용합니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`build.sh`는 항상 Debug 빌드와 테스트를 수행합니다. Release로 배포하려면 마지막에 위 명령을 실행하세요. 기본 `run.sh`는 `build/`에서 실행 파일을 찾으며 다른 폴더는 `MYPDF_BUILD_DIR`로 지정합니다. 문서 경로를 실행 인자로 전달할 수도 있습니다.

## 다른 컴퓨터에 배포

### macOS: 독립 앱 패키지

**macOS에서 Release 빌드를 완료한 뒤** 실행합니다. Debug 빌드라면 `--config Debug`를 사용하세요. 패키징 스크립트는 빌드를 수행하지 않습니다.

```bash
./scripts/package-macos.sh --config Release
open dist/MyPdf.app
```

Qt의 [macdeployqt](https://doc.qt.io/qt-6/macos-deployment.html), 필요한 플러그인, Poppler·qpdf와 연결된 비시스템 라이브러리를 앱에 포함합니다. qpdf는 `Contents/Helpers`에 배치합니다. 라이브러리 경로·아키텍처·로컬 서명과 앱/qpdf 시작을 검사하며 빌드 원본은 변경하지 않습니다.

기존 출력은 덮어쓰지 않습니다. 이전 결과를 이동하거나 새 출력 경로를 지정하세요.

```bash
./scripts/package-macos.sh --build-dir build --config Release --output dist/MyPdf-new.app
```

배포할 파일은 **`dist/MyPdf.app` 전체**입니다. 대상 Mac의 `~/Applications` 또는 `/Applications`로 복사합니다. 대상에는 Homebrew나 개발 도구가 필요하지 않지만 빌드와 호환되는 아키텍처·macOS 버전이 필요합니다. 시스템 폰트를 사용하며 universal 앱을 자동 생성하지 않습니다. ZIP으로 전달하려면 다음을 실행합니다.

```bash
ditto -c -k --sequesterRsrc --keepParent dist/MyPdf.app dist/MyPdf-macos.zip
```

현재 서명은 로컬 실행용 ad-hoc 방식입니다. 공개 배포용 Developer ID 서명·공증과 DMG 제작은 별도 작업입니다. 배포 전 Homebrew가 없는 호환 Mac에서 문서 열기·저장, DOCX 가져오기와 페이지·서명 편집을 확인하세요.

### Linux: 단일 AppImage

위의 **Release 빌드를 완료한 Linux 컴퓨터**에서 실행합니다. 추가 빌드 도구는 Python 3와 binutils입니다. 관리자 권한이나 FUSE는 패키징에 필요하지 않습니다.

```bash
sudo apt install python3 binutils
./scripts/package-linux.sh
```

결과는 `dist/MyPdf-0.2.0-x86_64.AppImage`와 `.sha256`, `.build.json`입니다. 버전과 아키텍처는 빌드에 따라 달라집니다. Qt·Poppler·qpdf·C++ 런타임과 필요한 플러그인을 포함하므로 받는 사람이 이 패키지를 별도로 설치할 필요는 없습니다. [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy)와 Qt 플러그인, AppImage 런타임은 최초 실행 시 공식 GitHub 배포에서 내려받고 게시된 SHA-256으로 검증합니다. 도구는 `build/appimage-tools`에 캐시하며, 같은 캐시가 있으면 다시 다운로드하지 않습니다. 최초 패키징에는 인터넷 연결이 필요합니다.

**AppImage 파일 하나**를 전달하세요. `.sha256`을 함께 제공하면 무결성을 확인할 수 있으며, `.build.json`은 빌드 환경·시스템 라이브러리·도구 버전 기록입니다. 사용자는 받은 파일이 있는 폴더에서 실행 권한을 주고 실행합니다.

```bash
sha256sum -c MyPdf-0.2.0-x86_64.AppImage.sha256
chmod +x MyPdf-0.2.0-x86_64.AppImage
./MyPdf-0.2.0-x86_64.AppImage
```

체크섬 검사는 `.sha256`도 받은 경우에 실행합니다. FUSE를 사용할 수 없는 환경에서는 설치 없이 추출 실행할 수 있습니다.

```bash
APPIMAGE_EXTRACT_AND_RUN=1 ./MyPdf-0.2.0-x86_64.AppImage
```

AppImage는 원하는 폴더에 보관하고 실행할 수 있습니다. 업데이트는 실행 중인 앱을 종료하고 새 AppImage로 교체합니다. 패키징은 기존 출력을 덮어쓰지 않으므로 새 경로를 지정할 수도 있습니다.

```bash
./scripts/package-linux.sh --config Release --build-dir build --output dist/MyPdf-new.AppImage
```

지원 아키텍처는 네이티브 `x86_64`와 `aarch64`이며 현재 로컬 검증은 x86_64입니다. 시스템의 glibc·그래픽 드라이버·X11/XWayland·폰트는 사용합니다. **모든 Linux 버전에서 실행되는 것은 아닙니다.** 넓게 배포하려면 지원할 가장 오래된 OS에서 빌드하세요. Ubuntu 26.04에서 만든 파일을 이전 Ubuntu에서 실행할 수 있다고 보장하지 않습니다. CI는 Ubuntu 24.04에서 AppImage를 만들고 Qt·Poppler·qpdf가 없는 컨테이너에서 실행·PDF 작업을 검사하도록 구성되어 있습니다. 배포 대상 OS에서도 문서 열기·저장·서명·페이지 편집을 확인하세요. deb·자동 업데이트는 제공하지 않습니다.

#### 사용자 폴더에 버전 없는 이름으로 설치

Release 빌드와 AppImage 패키징을 마친 뒤, 프로젝트 폴더에서 아래 명령으로 `~/.local/bin/MyPdf`에 복사할 수 있습니다. 실행 중인 MyPdf를 먼저 종료하세요. 관리자 권한은 필요하지 않으며 실행 권한도 함께 설정합니다. 원본 파일명은 실제 생성된 버전·아키텍처에 맞게 바꾸세요.

```bash
install -Dm755 dist/MyPdf-0.2.0-x86_64.AppImage "$HOME/.local/bin/MyPdf"
"$HOME/.local/bin/MyPdf"
```

AppImage는 확장자가 없어도 실행됩니다. `~/.local/bin`이 `PATH`에 있으면 어느 폴더에서든 `MyPdf`로 실행할 수 있습니다. 경로가 없다면 다음을 실행하고, 계속 적용하려면 `export` 줄을 사용하는 셸의 시작 설정 파일(예: Bash의 `~/.bashrc`)에도 추가하세요.

```bash
export PATH="$HOME/.local/bin:$PATH"
MyPdf
```

업데이트할 때는 앱을 종료한 뒤 새 AppImage를 같은 설치 명령으로 복사하면 됩니다. FUSE 없이 실행하려면 `APPIMAGE_EXTRACT_AND_RUN=1 "$HOME/.local/bin/MyPdf"`를 사용하세요.

### Windows: 런타임이 있는 환경

Windows의 `cmake --install`은 앱을 설치하며 DLL은 복사하지 않습니다. 대상에도 MSYS2 UCRT64와 위 패키지를 설치한 뒤 UCRT64 터미널에서 실행하세요. Windows 포터블 패키징과 설치 프로그램은 아직 제공하지 않습니다.

## 기본 사용법과 제한

상단의 서명·형광펜·페이지·맞춤 버튼은 버튼 또는 화살표를 클릭하면 메뉴가 열리는 동일한 방식으로 동작합니다.

| 작업 | 사용 방법 |
| --- | --- |
| 문서 열기·저장 | File → Open / Save / Save As. DOCX를 열면 PDF로 가져오며 첫 저장 시 PDF 경로를 선택합니다. |
| 텍스트·서명 추가 | 도구 모음의 Text / Sign을 선택한 뒤 페이지에 배치합니다. |
| 형광펜 | 상단 형광펜 메뉴에서 형광펜을 켜고 페이지 위를 드래그합니다. 같은 메뉴의 형광펜 설정에서 색상·굵기·불투명도를 바꾸며, Shift로 수평선을 그립니다. Esc로 종료하고 선택 도구로 표시를 선택해 이동·삭제하거나 더블클릭으로 설정을 수정합니다. |
| 추가한 개체 편집 | 클릭으로 선택, 드래그로 이동, 모서리로 크기 조절, 더블클릭으로 수정합니다. 방향키 이동, Shift로 큰 단위 이동, Delete/Backspace로 삭제합니다. Esc는 배치·이동 취소 또는 선택 해제입니다. |
| 페이지 구성 | Pages에서 이동·삽입·추출합니다. 왼쪽 미리보기에서 여러 페이지를 선택할 수 있습니다. |
| 언어·테마 | Settings에서 변경합니다. 영어 메뉴 이름 기준이며 언어 설정에 따라 번역됩니다. |

추가한 텍스트·서명·형광펜 표시는 저장 후 MyPdf에서 다시 선택·수정할 수 있습니다. 다른 PDF 뷰어에서도 표시됩니다. 기존 PDF에 이미 합쳐진 글자·서명은 개별 편집할 수 없습니다.

DOCX 가져오기는 기본 문단·스타일·목록·표·이미지를 지원하지만 Word와 글꼴·페이지 배치가 다를 수 있습니다. 복잡한 도형·수식·머리글·각주 등의 재현은 제한되며 저장 전에 배치를 확인하세요. 원본 Word 파일은 변경하지 않으며 `.doc`와 DOCX로 다시 저장하는 기능은 지원하지 않습니다.

서명은 시각적 서명이며 인증서 기반 전자서명이 아닙니다. OCR, 기존 PDF 본문 직접 편집, 일반 실행 취소/다시 실행, 암호로 잠긴 PDF는 지원하지 않습니다. 큰 문서는 처리 중 잠시 응답이 느려질 수 있습니다.

## 테스트와 프로젝트 구성

```bash
ctest --test-dir build --output-on-failure
python3 tests/package_macos_test.py
python3 tests/package_linux_test.py
```

첫 명령은 빌드된 앱의 테스트, 나머지는 Python 3로 실행하는 macOS·Linux 패키징 로직 테스트입니다. GUI 테스트는 macOS에서 Cocoa, 그 외 OS에서 offscreen을 기본 사용합니다.

| 경로 | 용도 |
| --- | --- |
| `src/` | PDF·DOCX 처리, UI, 아이콘, 언어 설정 |
| `translations/`, `assets/` | 번역·앱 아이콘 등 내장 리소스 |
| `scripts/`, `cmake/`, `packaging/` | 빌드·실행·설치·Linux/macOS 패키징 |
| `tests/` | 문서·UI·패키징 검증 |
| `build/`, `dist/` | 생성된 빌드 결과와 배포 패키지 |

[GitHub Actions](.github/workflows/build.yml)는 세 OS에서 빌드·테스트하며 Linux AppImage를 `MyPdf-linux-x86_64`, macOS ZIP을 `MyPdf-macos` 아티팩트로 보관합니다.

## 문서와 라이선스

`README.md`를 영어 기준 문서로 관리하며 독일어·한국어 문서는 같은 구성과 명령을 유지합니다. 기능·의존성·배포 방법 변경 시 세 문서를 함께 갱신합니다.

프로젝트 전체 라이선스 파일은 아직 없습니다. 외부 배포 전 프로젝트 라이선스를 정하고 Qt·Poppler·qpdf 및 포함 라이브러리의 라이선스·고지·소스 제공 조건을 확인하세요.
