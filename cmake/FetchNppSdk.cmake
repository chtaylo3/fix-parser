# Downloads the Notepad++ plugin SDK headers + docking framework at configure
# time instead of vendoring them. Pinned to a specific plugintemplate commit and
# verified by SHA-512. The original directory layout is preserved because the
# docking sources include "..\Notepad_plus_msgs.h" relative to DockingFeature/.
#
# Outputs:
#   FIXPARSER_NPP_SDK_DIR   - include root (contains PluginInterface.h, etc.)
#   FIXPARSER_NPP_SDK_SOURCES - .cpp files that must be compiled into the plugin

set(FIXPARSER_NPP_TEMPLATE_COMMIT "d180d263d2275a9fe0ca35a8715cb48152d40bd3"
    CACHE STRING "Pinned npp-plugins/plugintemplate commit for the SDK headers")

set(FIXPARSER_NPP_SDK_DIR "${CMAKE_BINARY_DIR}/npp-sdk"
    CACHE PATH "Directory where the Notepad++ SDK is downloaded")

# "relative/path=sha512". Paths mirror the upstream src/ layout (minus "src/").
set(_npp_files
  "PluginInterface.h=62b56d2a890d4b0b6c83c46e3196e08fd3e54b17024d0f953d41864ac1f0ef932c0e3436b0cce4c82abdb250e8120879845bf38b676f77f242d1ee2733869c05"
  "Notepad_plus_msgs.h=b601db8176837b09ab6586db50a949b597d4276abbf84fab3b5fce3387af40f4e72274bc5a148e60bbe5420ea34180755f7ce54c575dc7f99d3ecfe234115850"
  "Scintilla.h=fa5fe88a008127dd5c55757531a5865d5837e07a5fd528d07c05f19d1e83dd2170324b6c4832804592deb6b26ac49cc43ac1ce80038b3a9fb36f6c36cac3f597"
  "Sci_Position.h=89a30496d2ebb9eb6bcb329f2e9702f9324bae59644e8b2504d17d68d5c39cecc424c07816c7c301872283e6608fcbb46c0059d7098f73cd15dd6168b13b4a32"
  "menuCmdID.h=d367abbddb9f34334b5cd369b4f3dcecec47a8e7109441e427d5a53b7697f9a91e9a880c737db9c9418ba38f8bd137f380face80b11e4d7cc3bc9023cf9e02fd"
  "DockingFeature/Docking.h=cd9764adcc5242937d3d3fdb1da3af3b25df5c1ab3ea400a18ee0ff8f8ee12efac0ab7b048003c2e4c72b841704c19ca5a3ecd1834151eb9f2c1f63a130ea839"
  "DockingFeature/DockingDlgInterface.h=c25de26a8fd81e55206e601febbd8113a23ba6f0ab36fd246f1a268529b6709894c14ab338ce569a6ddd62d2d1f57dbd8ab808fee84a49abe58b51e7ee56c94a"
  "DockingFeature/StaticDialog.h=474d9431d3b8ab84278a19422ae38b81654f71aa9dfb22e324ece8cbec5b630e7e12dbef3a7925944b998970adfe98387b4ad52aac4e2a326473e425ffc79afe"
  "DockingFeature/StaticDialog.cpp=4d17a4b4c3a14bd5b93e8be2a27904eb2808f638d655d6849dfcfb7b5abe91fd6671a18d93cfccd0bf7fb6e982e1408b3daa286c1689ae15fce3e16242f80065"
  "DockingFeature/Window.h=5ff1aa35e2e626547310b5c4087718e513f997308c6ea38585062d98c0c487428312ddaa94786aacd1870a3a093f1e149bcf7187a78544133dc3600904eaba18"
  "DockingFeature/dockingResource.h=84fd297ea98e7a94f6e7d90d807e85ec623ddc2370d5bcf1f57adb76a06f531d94fc731dfc03c622d8a941c7b7d5b01155e1805c709cff5acd75a0c068fc9bc4"
)

set(_base "https://raw.githubusercontent.com/npp-plugins/plugintemplate/${FIXPARSER_NPP_TEMPLATE_COMMIT}/src")
set(FIXPARSER_NPP_SDK_SOURCES "")

foreach(_entry IN LISTS _npp_files)
  string(REPLACE "=" ";" _parts "${_entry}")
  list(GET _parts 0 _rel)
  list(GET _parts 1 _sha)
  set(_dest "${FIXPARSER_NPP_SDK_DIR}/${_rel}")

  if(NOT EXISTS "${_dest}")
    message(STATUS "Downloading Notepad++ SDK ${_rel}")
    file(DOWNLOAD "${_base}/${_rel}" "${_dest}"
         EXPECTED_HASH SHA512=${_sha}
         TLS_VERIFY ON
         STATUS _status)
    list(GET _status 0 _code)
    if(NOT _code EQUAL 0)
      list(GET _status 1 _msg)
      file(REMOVE "${_dest}")
      message(FATAL_ERROR "Failed to download ${_rel}: ${_msg}")
    endif()
  endif()

  if(_rel MATCHES "\\.cpp$")
    list(APPEND FIXPARSER_NPP_SDK_SOURCES "${_dest}")
  endif()
endforeach()
