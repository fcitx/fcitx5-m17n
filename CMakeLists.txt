cmake_minimum_required(VERSION 2.6)

project(fcitx-m17n)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(M17N REQUIRED)
find_package(Fcitx 4.2.7 REQUIRED)

option(ENABLE_TEST "standalone test program" Off)

_fcitx_add_uninstall_target()

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")
add_definitions(-D_GNU_SOURCE)
if(M17N_VERSION VERSION_LESS "1.6.3")
  set(NEED_MINPUT_LIST 1)
else()
  set(NEED_MINPUT_LIST 0)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(po)
add_subdirectory(im)
if(ENABLE_TEST)
  add_subdirectory(testmim)
endif()
