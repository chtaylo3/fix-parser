cmake_minimum_required(VERSION 3.21)

# vcpkg manifest mode: dependencies are declared in vcpkg.json and installed
# automatically at configure time when CMAKE_TOOLCHAIN_FILE points at vcpkg.
project(fix-parser LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FIXPARSER_BUILD_TESTS "Build unit tests" ON)

# With the x64-windows-static triplet, link the static CRT so the plugin DLL is
# self-contained (no vcruntime/ucrt/pugixml DLLs to deploy) and loadable by a
# stock portable Notepad++ — including Debug builds.
if(VCPKG_TARGET_TRIPLET MATCHES "static")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Core parser library — pure C++ + pugixml, no Win32 / Notepad++ dependencies.
# ---------------------------------------------------------------------------
find_package(pugixml CONFIG REQUIRED)

add_library(fixparser_core STATIC
  src/core/DelimiterNormalizer.cpp
  src/core/FixTokenizer.cpp
  src/core/MessageFramer.cpp
  src/core/PrettyPrinter.cpp
  src/core/FixParser.cpp
  src/core/Dictionary.cpp
  src/core/DictionarySelector.cpp
  src/core/DictionaryCache.cpp
)
target_include_directories(fixparser_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
target_link_libraries(fixparser_core PUBLIC pugixml::pugixml)

# ---------------------------------------------------------------------------
# Unit tests (Catch2 via vcpkg "tests" feature).
# ---------------------------------------------------------------------------
# QuickFIX dictionaries are downloaded (pinned + hash-verified), never vendored.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchDictionaries)

# ---------------------------------------------------------------------------
# Notepad++ plugin DLL (Windows only). The SDK headers are downloaded, not
# vendored (see cmake/FetchNppSdk.cmake).
# ---------------------------------------------------------------------------
if(WIN32)
  include(FetchNppSdk)

  add_library(fixparser_plugin SHARED
    src/plugin/dllmain.cpp
    src/plugin/PluginDefinition.cpp
    src/ui/FixPanel.cpp
    src/ui/FieldRenderer.cpp
    src/ui/FixPanel.rc
    ${FIXPARSER_NPP_SDK_SOURCES}
  )
  set_target_properties(fixparser_plugin PROPERTIES OUTPUT_NAME "FixParser" PREFIX "")
  target_include_directories(fixparser_plugin PRIVATE
    "${FIXPARSER_NPP_SDK_DIR}"
    "${FIXPARSER_NPP_SDK_DIR}/DockingFeature"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin"
  )
  target_link_libraries(fixparser_plugin PRIVATE
    fixparser_core comctl32 shlwapi user32 gdi32)
  target_compile_definitions(fixparser_plugin PRIVATE UNICODE _UNICODE)

  # Developer-only local debug environment (targets excluded from ALL / CI).
  include(LocalDevEnv)
endif()

if(FIXPARSER_BUILD_TESTS)
  enable_testing()
  find_package(Catch2 3 CONFIG REQUIRED)

  add_executable(fixparser_tests
    test/unit/DelimiterNormalizerTests.cpp
    test/unit/FixTokenizerTests.cpp
    test/unit/MessageFramerTests.cpp
    test/unit/PrettyPrinterTests.cpp
    test/unit/FixParserTests.cpp
    test/unit/DictionaryTests.cpp
    test/unit/DatasetTests.cpp
  )
  target_link_libraries(fixparser_tests PRIVATE fixparser_core Catch2::Catch2WithMain)
  target_compile_definitions(fixparser_tests PRIVATE
    FIXPARSER_DICT_DIR="${FIXPARSER_DICT_DIR}"
    FIXPARSER_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/data")

  include(Catch)
  catch_discover_tests(fixparser_tests)
endif()
