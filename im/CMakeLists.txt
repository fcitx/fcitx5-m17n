set(fcitx_m17n_sources
    engine.cpp
    overrideparser.cpp
    keysymname.cpp
    )

add_fcitx5_addon(m17n ${fcitx_m17n_sources})
target_link_libraries(m17n Fcitx5::Core Fcitx5::Config ${M17N_TARGET})
target_include_directories(m17n PRIVATE ${PROJECT_BINARY_DIR})
install(TARGETS m17n DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
configure_file(m17n.conf.in.in m17n.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/m17n.conf.in" m17n.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/m17n.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon" COMPONENT config)

install(FILES default DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/m17n" COMPONENT config)
