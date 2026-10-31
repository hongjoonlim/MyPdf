# Use the plugin from the Qt installation linked into this development build.
# Qt reads qt.conf from Contents/Resources when launching a macOS app bundle.
if(NOT TARGET Qt6::QCocoaIntegrationPlugin)
  find_package(Qt6QCocoaIntegrationPlugin QUIET CONFIG PATHS "${Qt6Gui_DIR}" NO_DEFAULT_PATH)
endif()
if(NOT TARGET Qt6::QCocoaIntegrationPlugin)
  message(FATAL_ERROR
    "The Cocoa plugin for the selected Qt installation was not found. "
    "Check Qt6_DIR / CMAKE_PREFIX_PATH and the Qt platform plugins installation.")
endif()

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/qt.conf"
  CONTENT "[Paths]\nPlugins = \"$<TARGET_FILE_DIR:Qt6::QCocoaIntegrationPlugin>/..\"\n")
add_custom_command(TARGET MyPdf POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:MyPdf>/Contents/Resources"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/qt.conf"
    "$<TARGET_BUNDLE_DIR:MyPdf>/Contents/Resources/qt.conf"
  VERBATIM)

# Packaging metadata is kept outside the development app bundle.
set(_mypdf_macdeployqt "")
if(TARGET Qt6::macdeployqt)
  set(_mypdf_macdeployqt "$<TARGET_FILE:Qt6::macdeployqt>")
elseif(TARGET Qt6::qmake)
  set(_mypdf_macdeployqt "$<TARGET_FILE_DIR:Qt6::qmake>/macdeployqt")
endif()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/package-tools.txt"
  CONTENT "$<TARGET_BUNDLE_DIR:MyPdf>\n${_mypdf_macdeployqt}\n${QPDF_EXECUTABLE}\n")
