# Provides the QuickFIX data dictionaries to the build.
#
# Default behavior: download them from a pinned upstream commit and verify by
# SHA-512 (FIXPARSER_FETCH_DICTIONARIES=ON). If the download fails (e.g. offline
# or air-gapped CI), or if fetching is disabled, fall back to the vendored copies
# committed under third_party/quickfix-spec/. Either way the files end up in
# ${FIXPARSER_DICT_DIR} for tests and packaging.

option(FIXPARSER_FETCH_DICTIONARIES
       "Download FIX dictionaries (fall back to vendored copies on failure)" ON)

set(FIXPARSER_QUICKFIX_COMMIT "386ce46e917ae494ab6e90b1be90fd421cdbe3f9"
    CACHE STRING "Pinned quickfix/quickfix commit for FIX dictionaries")

set(FIXPARSER_DICT_DIR "${CMAKE_BINARY_DIR}/dictionaries"
    CACHE PATH "Directory where FIX dictionaries are made available")

set(_vendor_dir "${CMAKE_SOURCE_DIR}/third_party/quickfix-spec")

# "name=sha512" entries (SHA-512 hex never contains '=', so it is a safe split).
set(_fixparser_dicts
  "FIX40.xml=4a0ec38a0bf12e2ae0f878a038abac6479a24b096bb84b253b6fc381f62c2bb699a982a459755d38aeb56e8e853941631942959c841952e94d899c0ee3c8f9cf"
  "FIX41.xml=8ad906734f96d3ad4411b36de20dab18f1a1cb571968f12578d928c307296dd8b861efbedf9967c26d423efcf17b1739e8de3166ac8f46a2287269fe764f84df"
  "FIX42.xml=4aaa12451cc646bdbd3f4ff7ab56f9729de8b547ee80bcf94bd2d78d72fc441788ac3496a43cea113c90136d21fd3270647ff919468149fce3fbfbc7c8593286"
  "FIX43.xml=7bd5de7e365bea2f7e3e6d9e9b32ecbd613ee79954cb3362d574cb3df6878df583da3adb0b49df4ee02fcd37891d9ff3e3e0a46ec0df2a4400baa1c8831c714b"
  "FIX44.xml=a0015e6ce9f1b4885f2ed1cfad07770825ad63d9d50faf23e27ef7b16c6429122cdf264857e958bcb83a7f7c72cf8375b8f94957bedd4245df94a750cab1bbd1"
  "FIX50.xml=593b90a3577525083a2b3cb7c3fad8ca8a439cc0fb5b563884038cb700b432c8ad8ec9951fa33169da15948b42b66702612ba80e9403578f8a5f77d0235023ee"
  "FIX50SP1.xml=42e3c9b419d140b69f52407b34c0ea9ee1c7affbf7466df79125f22588ee865d7109a12d29da738fa28b8656ee06a50c12660a03776aa2388dda5506663410ce"
  "FIX50SP2.xml=447d24f5a664f87e11b567d07f36bbb58e7d9e7deb93210efb3c18f5a5e5091f6700cacf46f6146e64b1e235a1cc7f7b3c2640f17ea7ea3dddf59a194d85ae5e"
  "FIXT11.xml=0f2a590ba159dcaffc9b55ec808fe9b53155c127b8a8e4d89fb1a9824f1f6c200e910fb42840f8be55b74c9fb6a1b497065d40a03770d66ba4b35e56c47d2d8c"
)

set(_base "https://raw.githubusercontent.com/quickfix/quickfix/${FIXPARSER_QUICKFIX_COMMIT}/spec")

file(MAKE_DIRECTORY "${FIXPARSER_DICT_DIR}")

foreach(_entry IN LISTS _fixparser_dicts)
  string(REPLACE "=" ";" _parts "${_entry}")
  list(GET _parts 0 _name)
  list(GET _parts 1 _sha)
  set(_dest "${FIXPARSER_DICT_DIR}/${_name}")
  set(_vendored "${_vendor_dir}/${_name}")

  if(EXISTS "${_dest}")
    continue()
  endif()

  set(_have FALSE)

  if(FIXPARSER_FETCH_DICTIONARIES)
    message(STATUS "Downloading FIX dictionary ${_name}")
    file(DOWNLOAD "${_base}/${_name}" "${_dest}"
         EXPECTED_HASH SHA512=${_sha}
         TLS_VERIFY ON
         STATUS _status)
    list(GET _status 0 _code)
    if(_code EQUAL 0)
      set(_have TRUE)
    else()
      list(GET _status 1 _msg)
      message(WARNING "Download of ${_name} failed (${_msg}); using vendored copy")
      file(REMOVE "${_dest}")
    endif()
  endif()

  if(NOT _have)
    if(EXISTS "${_vendored}")
      configure_file("${_vendored}" "${_dest}" COPYONLY)
      set(_have TRUE)
    endif()
  endif()

  if(NOT _have)
    message(FATAL_ERROR
      "Could not obtain FIX dictionary ${_name}: download failed and no vendored "
      "copy exists at ${_vendored}")
  endif()
endforeach()

# Keep the NOTICE alongside the dictionaries for packaging.
configure_file("${CMAKE_SOURCE_DIR}/resources/dictionaries/NOTICE.txt"
               "${FIXPARSER_DICT_DIR}/NOTICE.txt" COPYONLY)
