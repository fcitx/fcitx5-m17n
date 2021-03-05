set(fcitx_m17n_sources
    engine.cpp
    overrideparser.cpp
    keysymname.cpp
    )

add_library(m17n MODULE ${fcitx_m17n_sources})
target_link_libraries(m17n Fcitx5::Core Fcitx5::Config ${FMT_TARGET} PkgConfig::M17NGui)
target_include_directories(m17n PRIVATE ${PROJECT_BINARY_DIR})
set_target_properties(m17n PROPERTIES PREFIX "")
install(TARGETS m17n DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
configure_file(m17n.conf.in.in m17n.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/m17n.conf.in" m17n.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/m17n.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

install(FILES default DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/m17n")
