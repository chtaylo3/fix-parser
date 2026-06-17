# Script-mode (cmake -P) helper: download + extract a pinned portable Notepad++.
# Required -D args: NPP_DIR, NPP_URL, NPP_SHA256.

if(EXISTS "${NPP_DIR}/notepad++.exe")
  message(STATUS "Notepad++ already present at ${NPP_DIR}")
  return()
endif()

file(MAKE_DIRECTORY "${NPP_DIR}")
set(_zip "${NPP_DIR}/../npp-portable.zip")

message(STATUS "Downloading portable Notepad++ ...")
file(DOWNLOAD "${NPP_URL}" "${_zip}"
     EXPECTED_HASH SHA256=${NPP_SHA256}
     TLS_VERIFY ON
     SHOW_PROGRESS
     STATUS _status)
list(GET _status 0 _code)
if(NOT _code EQUAL 0)
  list(GET _status 1 _msg)
  file(REMOVE "${_zip}")
  message(FATAL_ERROR "Failed to download Notepad++: ${_msg}")
endif()

message(STATUS "Extracting Notepad++ to ${NPP_DIR}")
file(ARCHIVE_EXTRACT INPUT "${_zip}" DESTINATION "${NPP_DIR}")
file(REMOVE "${_zip}")

if(NOT EXISTS "${NPP_DIR}/notepad++.exe")
  message(FATAL_ERROR "notepad++.exe not found after extraction in ${NPP_DIR}")
endif()
message(STATUS "Notepad++ ready at ${NPP_DIR}/notepad++.exe")
