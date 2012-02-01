include_directories (${M17N_INCLUDE_DIR})

set(testmim_SOURCES
    testmim.cc
    )

add_executable(testmim ${testmim_SOURCES})
target_link_libraries (testmim ${M17N_LIBRARIES})