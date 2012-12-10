include_directories (${M17N_INCLUDE_DIR}
                     ${PROJECT_BINARY_DIR}
                     ${FCITX4_FCITX_INCLUDE_DIRS}
                     ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
                     ${FCITX4_FCITX_UTILS_INCLUDE_DIRS})

set( fcitx_m17n_sources
    fcitx-m17n.c
    keysymname.c
    config.c
    overrideparser.c
    )

fcitx_translate_add_sources(
    fcitx-m17n.h
    keyname.h
    keysymname.h
    minput_list.h
    overrideparser.h
    i18nname.c
    )

if (NEED_MINPUT_LIST)
    set(fcitx_m17n_sources ${fcitx_m17n_sources} minput_list.c)
endif (NEED_MINPUT_LIST)

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" )

fcitx_add_addon_full(m17n DESC
    SOURCES ${fcitx_m17n_sources}
    LINK_LIBS ${M17N_LIBRARIES}
)

install(FILES default DESTINATION ${FCITX4_PREFIX}/share/fcitx/m17n)
