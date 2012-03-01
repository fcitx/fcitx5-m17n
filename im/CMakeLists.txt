include_directories (${M17N_INCLUDE_DIR}
                     ${PROJECT_BINARY_DIR}
                     ${FCITX4_FCITX_INCLUDE_DIRS}
                     ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
                     ${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

set( fcitx_m17n_sources
    fcitx-m17n.c
    keyname.c
    m17nconfig.c
    )

if (NEED_MINPUT_LIST)
    set(fcitx_m17n_sources ${fcitx_m17n_sources} minput_list.c)
endif (NEED_MINPUT_LIST)

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" )

fcitx_add_addon(fcitx-m17n ${fcitx_m17n_sources} )
fcitx_add_addon_conf_file(fcitx-m17n.conf)
fcitx_add_configdesc_file(fcitx-m17n.desc)
target_link_libraries (fcitx-m17n ${M17N_LIBRARIES})
