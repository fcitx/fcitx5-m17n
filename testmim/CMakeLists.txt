include_directories (${M17N_INCLUDE_DIR}
                     ${PROJECT_BINARY_DIR}
                     ../im
                     )

set(testmim_SOURCES
    testmim.cc
    )
    
if (NEED_MINPUT_LIST)
    set(testmim_SOURCES ${testmim_SOURCES} ../im/minput_list.c)
endif (NEED_MINPUT_LIST)

add_executable(testmim ${testmim_SOURCES})
target_link_libraries (testmim ${M17N_LIBRARIES})