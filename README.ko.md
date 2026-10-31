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
| macOS | Apple Clang, Homebrew, 현재 기기의 아키텍처 | 사용자 Mac에서 빌드·테스트·실행 확인. 독립 패키징은 별도 Mac 검증 필요. |

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
brew install cmake ninja pkgconf qtbase qttranslations poppler-qt6 qpdf zlib vulkan-headers
./scripts/build.sh
./scripts/run.sh
```

`build.sh`가 Homebrew의 Qt와 Poppler 경로를 자동 지정합니다. Qt 6 바인딩이 필요하므로 `poppler`만 설치하지 말고 **poppler-qt6**를 사용하세요. 이 패키지는 keg-only입니다. [Homebrew 패키지 설명](https://formulae.brew.sh/formula/poppler-qt6)을 참고하세요. 결과물은 `build/MyPdf.app`이며 실제 실행 파일은 `build/MyPdf.app/Contents/MacOS/MyPdf`입니다. 포메라니안 `.icns` 아이콘을 포함합니다. Apple Silicon·Intel은 각각 현재 기기에 맞는 의존성을 사용하며, universal binary를 생성하지는 않습니다.

#### macOS: `Could not find POPPLER_INCLUDE_DIR ... poppler-qt6.h`

이 오류는 빌드에 **필수인 Poppler Qt 6 개발 헤더**를 찾지 못했다는 뜻입니다. 위의 선택적 Vulkan 메시지와 구분해야 합니다. Homebrew의 일반 `poppler` 설치만으로는 Qt 6 바인딩을 사용할 수 없습니다. [별도 `poppler-qt6` 패키지](https://formulae.brew.sh/formula/poppler-qt6)는 keg-only이므로 설치되어 있어도 기본 검색 경로에 없을 수 있습니다.

```bash
brew install poppler-qt6 pkgconf
brew list --versions poppler-qt6
ls "$(brew --prefix poppler-qt6)/include/poppler/qt6/poppler-qt6.h"
./scripts/build.sh
```

수정된 `build.sh`는 헤더와 라이브러리의 실제 존재를 확인하고 Homebrew의 `pkg-config` 검색 경로, `POPPLER_INCLUDE_DIR`, `POPPLER_LIBRARY`를 명시적으로 전달합니다. 기존 Mac 체크아웃에도 최신 스크립트를 반영하세요. 설치되었다고 나오는데 위 헤더가 없다면 `brew reinstall poppler-qt6`로 재설치합니다. Apple Silicon과 Intel/Rosetta용 Homebrew를 섞지 말고 Qt·Poppler·컴파일러를 같은 아키텍처로 사용하세요.

CMake를 직접 실행할 때는 다음처럼 지정할 수 있습니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)" \
  -DPOPPLER_INCLUDE_DIR="$(brew --prefix poppler-qt6)/include/poppler/qt6" \
  -DPOPPLER_LIBRARY="$(brew --prefix poppler-qt6)/lib/libpoppler-qt6.dylib"
cmake --build build --parallel
```

#### macOS: `Tests require qpdf on PATH or -DQPDF_EXECUTABLE=...`

CMake가 **qpdf 실행 파일**을 찾지 못한 경우입니다. qpdf는 테스트뿐 아니라 페이지 편집·서명·텍스트 합성에도 필요하므로 테스트를 끄는 것만으로 해결되지 않습니다. [Homebrew의 qpdf 패키지](https://formulae.brew.sh/formula/qpdf)를 설치하고 실행 파일을 확인하세요.

```bash
brew install qpdf
"$(brew --prefix qpdf)/bin/qpdf" --version
./scripts/build.sh
```

수정된 `build.sh`는 `MYPDF_QPDF`로 지정한 실행 파일, Homebrew의 `qpdf/bin/qpdf`, `PATH` 순서로 탐색하고 `-DQPDF_EXECUTABLE`을 CMake에 전달합니다. Mac 체크아웃에도 최신 스크립트를 반영하세요. 설치했는데 위 명령으로 실행되지 않으면 `brew reinstall qpdf`로 재설치합니다. `MYPDF_QPDF`가 설정되어 있으면 유효한 실행 파일 경로인지 확인하거나 해당 설정을 해제하세요. CMake 직접 실행 시에는 기존 configure 명령에 `-DQPDF_EXECUTABLE="$(brew --prefix qpdf)/bin/qpdf"`를 추가합니다.

#### macOS: GUI 테스트 여러 개가 `Subprocess aborted`로 종료되는 경우

`native_docx`, `pdf_operations`, `localization`, `signature_cancellation`, `editable_objects`, `object_editing`은 모두 `QApplication`을 생성합니다. `tool_discovery`만 통과하고 이 테스트들이 전부 중단되면 Qt 플랫폼 플러그인 등 공통 GUI 초기화 실패가 의심되지만, 실패 목록만으로 확정할 수는 없습니다. 빌드 성공과 테스트 통과는 별개입니다.

기존 설정은 모든 GUI 테스트에 `offscreen`을 강제해 macOS에서 `Could not find the Qt platform plugin "offscreen"`으로 중단될 수 있었습니다. 수정된 설정은 macOS에서 기본 `cocoa`, 그 외 운영체제에서 `offscreen`을 사용합니다. 연결한 Qt의 플러그인 CMake 타깃에서 실제 경로를 찾으면 테스트에만 `QT_QPA_PLATFORM_PLUGIN_PATH`와 `QT_PLUGIN_PATH`를 전달합니다. 전역 셸 설정은 변경하지 않습니다.

Mac 체크아웃에 수정된 `CMakeLists.txt`, `cmake/test_environment.cmake`, `scripts/build.sh`를 반영한 뒤 다음을 실행하세요. `cocoa` 테스트는 로그인된 macOS 데스크톱 세션에서 실행하며 창이나 대화상자가 잠깐 표시될 수 있습니다.

```bash
MYPDF_TEST_QPA_PLATFORM=cocoa ./scripts/build.sh
```

`offscreen` 플러그인을 포함한 Qt로 화면 없이 테스트하려면 `MYPDF_TEST_QPA_PLATFORM=offscreen`을 대신 지정할 수 있습니다. 플러그인 경로를 직접 지정해야 할 때는 `MYPDF_TEST_QT_PLUGIN_DIR`에 같은 Qt 버전·아키텍처의 `platforms` 디렉터리를 설정합니다. 두 변수는 빌드 스크립트가 CMake 옵션으로 전달하므로 선택값이 빌드 캐시에 남습니다.

그래도 중단되면 다음으로 실제 플러그인 탐색 로그를 확인하세요.

```bash
QT_DEBUG_PLUGINS=1 ctest --test-dir build -R '^pdf_operations$' -V
```

전체 테스트 로그는 `build/Testing/Temporary/LastTest.log`에 있습니다. `Could not find/load the Qt platform plugin`, `Library not loaded`, `incompatible architecture` 등의 **실제 출력**을 기준으로 원인을 진단해야 합니다. 플러그인 경로가 다른 Qt/Python 환경을 가리키거나 Qt와 플러그인의 버전·아키텍처가 맞지 않는지도 확인합니다. `QT_DEBUG_PLUGINS`는 [Qt의 공식 플러그인 진단 옵션](https://doc.qt.io/qt-6/deployment-plugins.html)입니다. CTest의 테스트별 환경 설정이 우선하므로, 부모 셸에서 `QT_QPA_PLATFORM=cocoa`만 지정해도 해당 테스트 설정은 바뀌지 않습니다. 로그 확인 없이 테스트를 끄거나 패키지를 추가 설치하는 것으로 해결되었다고 판단하지 마세요.

#### macOS: 테스트가 `Signature Image` 파일 선택 창에서 기다리는 경우

사용자가 파일을 선택해야 하는 테스트가 아닙니다. Qt의 `selectFile()`은 파일명 입력칸에 포커스가 있으면 입력 내용을 바꾸지 않으므로, 기존의 즉시 선택·확인 방식이 파일 선택 창을 열어 둔 채 기다릴 수 있었습니다. 파일 모델 로딩도 비동기로 이루어집니다. 수정된 테스트는 창·모델을 기다리고 파일명 입력칸을 채운 뒤 실제 파일 선택 검증을 거쳐 확인합니다. 파일 선택 자동화는 10초, GUI 테스트 전체는 120초의 제한 시간을 둡니다.

Mac 체크아웃에 `tests/file_dialog_automation.h`, `tests/object_edit_test.cpp`, `tests/docx_test.cpp`, `CMakeLists.txt`를 반영한 뒤 다시 빌드하세요.

```bash
MYPDF_TEST_QPA_PLATFORM=cocoa ./scripts/build.sh
ctest --test-dir build -R '^(object_editing|native_docx)$' -V --timeout 120
```

창에 직접 입력할 필요는 없습니다. 실패하면 시간 초과 메시지의 파일 경로·활성 창 제목과 `LastTest.log`를 확인합니다. 이 변경은 테스트 자동화에 대한 것으로, 실제 프로그램의 서명 이미지 선택은 계속 사용자가 수행합니다.

#### macOS: 테스트는 통과하지만 실행 시 `cocoa` 플러그인을 찾지 못하는 경우

테스트용 플러그인 경로 설정과 앱 실행 설정은 별개입니다. CMake는 이제 빌드에 연결한 Qt의 Cocoa 플러그인 경로를 `MyPdf.app/Contents/Resources/qt.conf`에 기록합니다. Qt는 [앱 번들의 이 설정 파일](https://doc.qt.io/qt-6/qt-conf.html)을 읽으므로 `run.sh` 실행과 앱 직접 실행에 모두 적용됩니다. 수정된 `CMakeLists.txt`와 `cmake/macos_runtime.cmake`를 Mac에도 반영하고 다시 빌드하세요.

```bash
./scripts/build.sh
./scripts/run.sh
```

기존에 설치한 앱을 사용한다면 재빌드한 `.app`으로 다시 설치하세요. 이 설정은 해당 Mac에 설치된 Qt를 참조하는 로컬 개발용입니다. 다른 Mac에 앱만 복사하려면 Qt 라이브러리와 플러그인을 포함하는 별도의 배포 작업이 필요합니다. 계속 실패하면 `QT_DEBUG_PLUGINS=1 ./scripts/run.sh` 출력에서 실제 플러그인 경로와 로딩 오류를 확인하세요.

#### macOS: `Could NOT find WrapVulkanHeaders (missing Vulkan_INCLUDE_DIR)`

Qt가 **선택적인 Vulkan 헤더**를 찾지 못했다는 메시지이며, 이 한 줄이 반드시 빌드 실패를 뜻하지는 않습니다. MyPdf는 Qt Widgets·QPainter·Poppler를 사용하고 Vulkan API를 직접 호출하지 않습니다. 이후 `Configuring done`, `Generating done`이 나오면 구성은 성공한 것입니다. 구성이 중단된다면 뒤이어 나오는 `CMake Error`와 다른 의존성 오류도 확인하세요. Qt도 이 의존성을 [구성 소스에서 선택 항목으로 표시](https://github.com/qt/qtbase/blob/6.10/src/gui/configure.cmake#L74-L75)합니다.

위 macOS 설치 명령에는 헤더 탐색을 해결하는 [Homebrew의 `vulkan-headers`](https://formulae.brew.sh/formula/vulkan-headers)를 포함했습니다. 이전 README로 설치했다면 다음을 실행하세요.

```bash
brew install vulkan-headers
./scripts/build.sh
```

`build.sh`는 Homebrew에 설치된 헤더를 감지해 `Vulkan_INCLUDE_DIR`를 자동 전달합니다. CMake를 직접 실행한다면 다음처럼 지정합니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)" \
  -DVulkan_INCLUDE_DIR="$(brew --prefix vulkan-headers)/include"
cmake --build build --parallel
```

이 메시지를 해결하는 데는 헤더 패키지만 필요하며, MyPdf용으로 Vulkan SDK·Vulkan 로더·MoltenVK 런타임까지 설치할 필요는 없습니다. 다른 Qt·Poppler 의존성 오류는 별도로 해결해야 합니다.


### 공통 명령과 설치

`build.sh`는 Debug 빌드 후 테스트를 실행하고, `run.sh`는 운영체제에 맞는 실행 파일을 선택합니다. Release 빌드는 아래 명령을 사용하되 Windows에서는 configure 명령에 `-G Ninja -DCMAKE_PREFIX_PATH="$MINGW_PREFIX"`, macOS에서는 `-DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix poppler-qt6);$(brew --prefix zlib)"`를 추가해 같은 의존성 경로를 지정합니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./scripts/run.sh '/path/to/document.pdf'
```

`run.sh`는 스크립트 위치를 기준으로 상대 경로로 실행 파일을 찾고 현재 작업 폴더는 변경하지 않습니다. 따라서 문서의 상대 경로는 스크립트를 실행한 폴더 기준으로 처리합니다. macOS·Windows의 `build/Debug`, `build/Release` 같은 구성별 출력 폴더도 지원합니다. 다른 빌드 폴더는 `MYPDF_BUILD_DIR`로 지정할 수 있습니다(현재 폴더 기준 상대 경로 또는 절대 경로). 빌드 폴더와 CMake 캐시는 운영체제·컴퓨터별로 새로 생성해야 합니다.

개발용 설치는 `cmake --install build --prefix /path/to/install`로 수행합니다. macOS는 `MyPdf.app`, Windows는 `bin/MyPdf.exe`, Linux는 실행 파일과 데스크톱 런처·아이콘을 설치합니다. **의존성을 모두 포함한 독립 배포 패키지는 아닙니다.** Qt 플러그인·라이브러리, Poppler 의존성, qpdf와 그 의존성이 실행 환경에 있어야 합니다. macOS 독립 패키징은 아래 별도 스크립트를 사용합니다. Windows 자동 DLL 배포와 설치 프로그램은 구현하지 않았습니다.

qpdf 탐색 순서는 `MYPDF_QPDF`, 앱 실행 파일 옆, `../Helpers/qpdf` 또는 `qpdf.exe`, `PATH`, CMake 구성 시 발견한 경로입니다. 마지막 경로는 빌드한 컴퓨터에 종속된 개발용 대체 경로입니다. Finder에서 실행하려면 지원 경로에 qpdf를 두거나 설정된 터미널에서 `run.sh`를 사용하세요. 탐색을 직접 지정하려면 `MYPDF_QPDF`에 추가 인자 없이 실행 파일 경로만 넣습니다.

[`.github/workflows/build.yml`](.github/workflows/build.yml)은 GitHub에 올렸을 때 Ubuntu·Windows UCRT64·macOS에서 빌드와 테스트를 실행합니다. 워크플로 파일을 추가한 것과 실제 원격 작업이 통과한 것은 구분해야 합니다.

### macOS: 일반 앱처럼 로컬에 설치

빌드한 Mac에서 사용할 때는 프로젝트 폴더에서 아래 명령으로 사용자 응용 프로그램 폴더에 설치합니다. 관리자 권한은 필요하지 않습니다.

```bash
mkdir -p "$HOME/Applications"
cmake --install build --prefix "$HOME/Applications"
open "$HOME/Applications/MyPdf.app"
```

Finder에서 `~/Applications/MyPdf.app`을 더블클릭해 실행할 수 있습니다. Dock 아이콘을 우클릭하고 **옵션 → Dock에 유지**를 선택하면 다음부터 편하게 실행할 수 있습니다. `run.sh`는 필요하지 않습니다. 이 개발용 설치는 Homebrew의 Qt·Poppler·qpdf를 계속 사용하므로 해당 의존성을 유지하세요. 업데이트는 앱을 종료한 뒤 `./scripts/build.sh`와 위 설치 명령을 다시 실행합니다.

### macOS: 라이브러리를 포함하는 독립 앱 패키징

최신 CMake 파일을 반영한 뒤 Mac에서 실행합니다. Python 3, Xcode Command Line Tools, 빌드에 사용한 Qt의 `macdeployqt`, qpdf가 필요합니다. 앞의 Homebrew/개발 도구 설치와 빌드를 먼저 완료하세요.

```bash
./scripts/build.sh
./scripts/package-macos.sh
open dist/MyPdf.app
```

결과물은 `dist/MyPdf.app`입니다. 스크립트는 빌드 결과의 복사본에 [Qt 공식 `macdeployqt`](https://doc.qt.io/qt-6/macos-deployment.html)를 적용하고, `Contents/Helpers/qpdf`와 Qt·Poppler·qpdf가 연결하는 비시스템 라이브러리를 재귀적으로 포함합니다. 플러그인 경로와 라이브러리 참조를 앱 내부의 상대 경로로 바꾸고, 외부 참조·누락 파일·아키텍처를 검사합니다. 로컬 실행용 ad-hoc 서명과 서명 검증을 수행한 뒤, Qt/DYLD 환경 변수와 Homebrew PATH 없이 앱 시작 및 qpdf 실행을 확인합니다. 빌드 폴더의 개발용 `.app`은 변경하지 않습니다. 기존 출력은 덮어쓰지 않으므로 재패키징할 때 이전 결과를 이동하거나 `--output`으로 새 경로를 지정하세요.

패키징은 Cocoa·래스터 이미지·macOS 스타일 플러그인을 명시적으로 선택합니다. MyPdf에서 사용하지 않는 Qt Virtual Keyboard/QML, QtPdf 이미지 플러그인과 SVG 아이콘 엔진은 포함하지 않습니다. `Cannot resolve @rpath/QtVirtualKeyboardQml.framework/...` 오류가 있었다면 최신 `scripts/package_macos.py`를 반영하고 패키징을 다시 실행하세요. 같은 Qt의 `qmake`로 라이브러리·플러그인 경로를 조회하므로 추가 Qt 모듈을 설치할 필요는 없습니다. 필요한 의존성을 찾지 못하면 검색한 경로를 오류에 표시합니다.

```bash
./scripts/package-macos.sh --build-dir build --config Debug --output dist/MyPdf-new.app
```

Release 또는 다중 구성 빌드는 실제 빌드 구성에 맞춰 `--config Release`를 지정합니다. 스크립트가 빌드를 수행하지는 않습니다. 패키지의 앱 전체를 Finder에서 `~/Applications`로 복사하면 됩니다. 설치된 이전 앱은 먼저 종료하세요. 패키징된 앱은 Homebrew 런타임을 요구하지 않지만 macOS 시스템 라이브러리·시스템 폰트는 사용합니다. 문서의 폰트는 별도로 포함하지 않습니다. Apple Silicon/Intel과 지원되는 최소 macOS 버전은 빌드한 의존성에 따르며 universal 앱으로 변환하지 않습니다. 최종 확인은 Homebrew가 없는 호환 Mac에서 PDF 열기·저장, DOCX 가져오기, 서명·페이지 편집을 실행하세요. Linux에서는 macOS 패키징을 실행할 수 없습니다.

일반 설치 명령은 개발용 `.app`을 설치하므로, 독립 패키지를 설치할 때는 반드시 `dist`의 결과물을 사용하세요. GitHub macOS CI에도 패키징 단계를 추가했으며 결과를 `MyPdf-macos` ZIP 아티팩트로 보관합니다. 공개 배포용 Developer ID 서명·공증 및 DMG 설치 이미지는 자동화하지 않았습니다. 다른 사용자에게 배포하기 전에는 포함 라이브러리의 라이선스·고지·소스 제공 의무도 확인해야 합니다.


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
