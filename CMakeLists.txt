cmake_minimum_required(VERSION 3.21)

# vcpkg manifest mode: dependencies are declared in vcpkg.json and installed
# automatically at configure time when CMAKE_TOOLCHAIN_FILE points at vcpkg.
project(fix-parser LANGUAGES CXX VERSION 0.1.0) # x-release-please-version

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FIXPARSER_BUILD_TESTS  "Build unit tests" ON)
option(FIXPARSER_BUILD_PLUGIN "Build the Notepad++ plugin DLL (Windows only)" ON)

# With the x64-windows-static triplet, link the static CRT so the plugin DLL is
# self-contained (no vcruntime/ucrt/pugixml DLLs to deploy) and loadable by a
# stock portable Notepad++ — including Debug builds.
if(VCPKG_TARGET_TRIPLET MATCHES "static")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Project-wide baseline warnings (applies to every target, third-party included).
if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Hardening knobs for first-party code (the parser core / tests). Applied
# per-target via the helpers below so third-party sources (pugixml, the
# Notepad++ SDK) are not held to the same bar.
# ---------------------------------------------------------------------------
option(FIXPARSER_WARNINGS_AS_ERRORS "Treat warnings as errors for first-party targets" OFF)
option(FIXPARSER_ENABLE_SANITIZERS  "Build first-party targets with ASan + UBSan" OFF)
option(FIXPARSER_MSVC_ANALYZE       "Enable MSVC /analyze on first-party targets" OFF)
option(FIXPARSER_ENABLE_CLANG_TIDY  "Run clang-tidy on the parser core during build" OFF)

# Strict warnings (on top of the project-wide baseline) for one target.
#   MSVC:        /W4 /permissive- already global; add /analyze and/or /WX.
#   GCC/Clang:   -Wall -Wextra already global; add the stricter set + -Werror.
function(fixparser_harden target)
  if(MSVC)
    if(FIXPARSER_MSVC_ANALYZE)
      target_compile_options(${target} PRIVATE /analyze)
    endif()
    if(FIXPARSER_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE
      -Wpedantic -Wconversion -Wshadow -Wformat=2)
    if(FIXPARSER_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

# AddressSanitizer + UndefinedBehaviorSanitizer for one target. The single
# highest-value tooling for a parser (buffer overruns, UAF, signed overflow,
# OOB shifts, misaligned access, ...). UBSan is unavailable on MSVC, which gets
# ASan only (/fsanitize=address).
function(fixparser_sanitize target)
  if(NOT FIXPARSER_ENABLE_SANITIZERS)
    return()
  endif()
  if(MSVC)
    # MSVC (and clang-cl, which also reports MSVC) build cleanly with native
    # AddressSanitizer. ASan needs a non-debug CRT, so the sanitize preset uses
    # RelWithDebInfo. UBSan is not available with the MSVC driver (it needs the
    # GNU-style clang/gcc driver), so it is not enabled here. The deps are built
    # WITH ASan via the x64-windows-static-asan overlay triplet, so STL container
    # annotations are consistent on both sides -- no _DISABLE_* macros needed
    # (which keeps this working across Visual Studio toolset versions).
    target_compile_options(${target} PRIVATE /fsanitize=address)
  else()
    # GCC / Clang with the GNU driver: AddressSanitizer + UndefinedBehaviorSanitizer.
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target}    PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

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
fixparser_harden(fixparser_core)
fixparser_sanitize(fixparser_core)

# clang-tidy runs per translation unit during the build (checks in .clang-tidy).
# Scoped to the core target so third-party sources are never linted.
if(FIXPARSER_ENABLE_CLANG_TIDY)
  find_program(FIXPARSER_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
  set_target_properties(fixparser_core PROPERTIES
    CXX_CLANG_TIDY "${FIXPARSER_CLANG_TIDY_EXE}")
endif()

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
if(WIN32 AND FIXPARSER_BUILD_PLUGIN)
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
  # Tests must be compiled + linked with the same sanitizers as the code under
  # test, so the Catch2 run actually exercises ASan/UBSan instrumentation.
  fixparser_sanitize(fixparser_tests)
  target_compile_definitions(fixparser_tests PRIVATE
    FIXPARSER_DICT_DIR="${FIXPARSER_DICT_DIR}"
    FIXPARSER_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/data")

  include(Catch)
  catch_discover_tests(fixparser_tests)
endif()

# ---------------------------------------------------------------------------
# libFuzzer harness (clang / clang-cl only). Coverage-guided fuzzing of the
# parser core under ASan. Built via the "fuzz" preset; run manually for hours.
# ---------------------------------------------------------------------------
if(FIXPARSER_BUILD_FUZZER)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
      "FIXPARSER_BUILD_FUZZER needs clang/clang-cl (libFuzzer). Use the 'fuzz' preset.")
  endif()

  # Coverage-instrument the code under test (no fuzzer main in the library).
  # The fuzzer uses clang-cl against non-instrumented vcpkg deps, so disable the
  # MSVC-STL container annotations to keep both sides consistent (clang-cl honors
  # these). The fuzz workflow runs on the matching VS 2022 toolset.
  target_compile_options(fixparser_core PRIVATE -fsanitize=fuzzer-no-link,address /Oy-)
  target_compile_definitions(fixparser_core PRIVATE _DISABLE_STRING_ANNOTATION=1 _DISABLE_VECTOR_ANNOTATION=1)

  add_executable(fixparser_fuzzer src/fuzz/fuzz_parser.cpp)
  target_link_libraries(fixparser_fuzzer PRIVATE fixparser_core)
  target_compile_options(fixparser_fuzzer PRIVATE -fsanitize=fuzzer,address /Oy-)
  target_compile_definitions(fixparser_fuzzer PRIVATE _DISABLE_STRING_ANNOTATION=1 _DISABLE_VECTOR_ANNOTATION=1)

  # CMake links via lld-link directly (it ignores -fsanitize), so add clang's
  # runtime lib dir and link the libFuzzer + ASan runtimes explicitly, and copy
  # the ASan runtime DLL next to the fuzzer so it launches.
  execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-resource-dir
    OUTPUT_VARIABLE FIXPARSER_CLANG_RT OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(FIXPARSER_CLANG_RT_WIN "${FIXPARSER_CLANG_RT}/lib/windows")
  target_link_directories(fixparser_fuzzer PRIVATE "${FIXPARSER_CLANG_RT_WIN}")
  target_link_libraries(fixparser_fuzzer PRIVATE
    clang_rt.fuzzer-x86_64
    clang_rt.asan_dynamic-x86_64
    clang_rt.asan_dynamic_runtime_thunk-x86_64)
  add_custom_command(TARGET fixparser_fuzzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${FIXPARSER_CLANG_RT_WIN}/clang_rt.asan_dynamic-x86_64.dll"
      "$<TARGET_FILE_DIR:fixparser_fuzzer>"
    VERBATIM)
endif()
