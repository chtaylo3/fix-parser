# vcpkg overlay triplet: x64 static-CRT build with AddressSanitizer.
#
# Used by the "sanitize" and "fuzz" presets so that vcpkg dependencies
# (pugixml, Catch2) are compiled WITH ASan, matching our instrumented code.
# This keeps the MSVC-STL container annotations consistent on both sides, which
# avoids the LNK2038 annotate_string/vector/optional mismatch without depending
# on version-specific opt-out macros — so it works on any Visual Studio toolset.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_C_FLAGS "/fsanitize=address")
set(VCPKG_CXX_FLAGS "/fsanitize=address")
