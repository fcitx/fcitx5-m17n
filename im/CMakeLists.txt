include_directories (${M17N_INCLUDE_DIR})
include_directories (${FCITX4_FCITX_INCLUDE_DIRS})
include_directories (${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories (${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

set( fcitx_m17n_sources
    fcitx-m17n.c
    keyname.c
    )

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" )

fcitx_add_addon(fcitx-m17n ${fcitx_m17n_sources} )
fcitx_add_addon_conf_file(fcitx-m17n.conf)
target_link_libraries (fcitx-m17n ${M17N_LIBRARIES})
