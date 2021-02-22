add_custom_target(copy-addon DEPENDS m17n.conf.in-fmt)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/im/m17n.conf ${CMAKE_CURRENT_BINARY_DIR}/m17n.conf)
