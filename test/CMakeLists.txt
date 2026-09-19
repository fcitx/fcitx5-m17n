
configure_file(testdir.h.in ${CMAKE_CURRENT_BINARY_DIR}/testdir.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(addon)
add_executable(testm17n testm17n.cpp)
target_link_libraries(testm17n Fcitx5::Core Fcitx5::Module::TestFrontend)
add_dependencies(testm17n m17n copy-addon)

add_custom_command(TARGET testm17n POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/m17n
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/im/default ${CMAKE_CURRENT_BINARY_DIR}/m17n/default)

add_test(NAME testm17n COMMAND testm17n)
