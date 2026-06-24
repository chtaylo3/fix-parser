# Developer-only local test environment. None of these targets are part of ALL,
# so normal builds and CI never run them. They wire up a one-command
# build -> deploy -> F5 loop against a real portable Notepad++.
#
#   npp_fetch       download + extract portable Notepad++ into tools/npp-<arch>
#   gen_sample_log  generate large sample FIX logs into tools/sample
#   deploy_local    copy the plugin DLL + PDB + dictionaries into tools/npp-<arch>
#   local_env       umbrella: all of the above (use as the VS Code preLaunchTask)
#
# Architecture follows the configure's triplet, so the x64 build tree wires up a
# 64-bit portable Notepad++ and the x86 build tree a 32-bit one. A plugin only
# loads in a Notepad++ of the matching bitness, so they live in separate dirs and
# the VS Code F5 "arch" picker selects which one to build, deploy, and launch.
# Notepad++ publishes only Release builds; debugging the plugin comes from the
# plugin itself being built Debug (PDBs) via the *-debug preset, not from N++.

set(FIXPARSER_NPP_VERSION "8.9.6.4" CACHE STRING "Portable Notepad++ version")

if(VCPKG_TARGET_TRIPLET MATCHES "^x86")
  set(_npp_arch "win32")
  set(_npp_zip  "npp.${FIXPARSER_NPP_VERSION}.portable.zip")
  set(_npp_sha  "93f19ca5a5f2b857dd9397d080b5bfc3552e72bea26e4beb701efea2101f3ac0")
else()
  set(_npp_arch "x64")
  set(_npp_zip  "npp.${FIXPARSER_NPP_VERSION}.portable.x64.zip")
  set(_npp_sha  "7b3618195757eed0d47debc28661fe998e68d3822a06a3621ee669ee358fc952")
endif()
set(_npp_url "https://github.com/notepad-plus-plus/notepad-plus-plus/releases/download/v${FIXPARSER_NPP_VERSION}/${_npp_zip}")

set(FIXPARSER_MULTILINE_COUNT "25000" CACHE STRING "Messages in the moderate sample log")
set(FIXPARSER_SINGLELINE_MB "50" CACHE STRING "Size (MB) of the single-line stress log")

set(_tools "${CMAKE_SOURCE_DIR}/tools")
set(_npp_dir "${_tools}/npp-${_npp_arch}")
set(_sample_dir "${_tools}/sample")
set(_plugin_dest "${_npp_dir}/plugins/FixParser")

add_custom_target(npp_fetch
  COMMAND ${CMAKE_COMMAND}
          -DNPP_DIR=${_npp_dir}
          -DNPP_URL=${_npp_url}
          -DNPP_SHA256=${_npp_sha}
          -P "${CMAKE_SOURCE_DIR}/cmake/FetchNpp.cmake"
  COMMENT "Fetching ${_npp_arch} portable Notepad++ ${FIXPARSER_NPP_VERSION}"
  VERBATIM)

add_custom_target(gen_sample_log
  COMMAND ${CMAKE_COMMAND} -E make_directory "${_sample_dir}"
  COMMAND ${CMAKE_COMMAND}
          -DSRC44=${CMAKE_SOURCE_DIR}/test/data/fixsim/fix44.txt
          -DSRC42=${CMAKE_SOURCE_DIR}/test/data/fixsim/fix42.txt
          -DOUT_MULTILINE=${_sample_dir}/sample-multiline.fix
          -DOUT_SINGLELINE=${_sample_dir}/sample-singleline.fix
          -DMULTILINE_COUNT=${FIXPARSER_MULTILINE_COUNT}
          -DSINGLELINE_MB=${FIXPARSER_SINGLELINE_MB}
          -P "${CMAKE_SOURCE_DIR}/cmake/GenSampleLog.cmake"
  COMMENT "Generating sample FIX logs in ${_sample_dir}"
  VERBATIM)

# Copy the freshly built plugin (DLL + PDB) and the dictionaries into the
# portable Notepad++ plugin folder so it loads on next launch.
add_custom_target(deploy_local
  COMMAND ${CMAKE_COMMAND} -E make_directory "${_plugin_dest}/dictionaries"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:fixparser_plugin>" "${_plugin_dest}/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_PDB_FILE:fixparser_plugin>" "${_plugin_dest}/"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${FIXPARSER_DICT_DIR}" "${_plugin_dest}/dictionaries"
  DEPENDS fixparser_plugin
  COMMENT "Deploying FixParser into ${_plugin_dest}"
  VERBATIM)

add_custom_target(local_env
  DEPENDS npp_fetch gen_sample_log deploy_local
  COMMENT "Preparing local Notepad++ debug environment")
