
configure_file(testdir.h.in ${CMAKE_CURRENT_BINARY_DIR}/testdir.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(addon)
add_executable(testm17n testm17n.cpp)
target_link_libraries(testm17n Fcitx5::Core Fcitx5::Module::TestFrontend Pthread::Pthread)
add_dependencies(testm17n m17n copy-addon)
add_test(testm17n testm17n)
