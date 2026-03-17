message("\n*** cmake lib_ut: so users can call this CMakeLists.txt in their own project ***")

file(GLOB_RECURSE  list_src  CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
add_library(lib_ut  OBJECT  ${list_src})

target_include_directories(lib_ut  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/domino
    ${CMAKE_CURRENT_SOURCE_DIR}/log
    ${CMAKE_CURRENT_SOURCE_DIR}/msg_self
    ${CMAKE_CURRENT_SOURCE_DIR}/obj_anywhere
    ${CMAKE_CURRENT_SOURCE_DIR}/safe_mem
    ${CMAKE_CURRENT_SOURCE_DIR}/thread
)

target_link_libraries(lib_ut  PUBLIC
    lib_domino
    gtest
    gmock
)
