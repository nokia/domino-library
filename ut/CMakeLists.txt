message("\n*** cmake lib_ut: so users can call this CMakeLists.txt in their own project ***")

# src
file(GLOB_RECURSE  list_src
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
add_library(lib_ut  OBJECT  ${list_src})

# include
file(GLOB  list_include
  "${CMAKE_CURRENT_SOURCE_DIR}/*"
  "${CMAKE_CURRENT_SOURCE_DIR}/../src/*"
)
list(FILTER  list_include  EXCLUDE  REGEX  ".*CMakeLists.*")
target_include_directories(lib_ut  PUBLIC
  ${list_include}
)

target_link_libraries(lib_ut PUBLIC
  gtest
  gmock
)
