message("\n*** cmake lib_domino: so users can call this CMakeLists.txt in their own project ***")
# src
file(GLOB_RECURSE  list_src
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
add_library(lib_domino  ${list_src})

# include dir
file(GLOB  list_include
  "${CMAKE_CURRENT_SOURCE_DIR}/*"
)
list(FILTER  list_include  EXCLUDE  REGEX  ".*CMakeLists.*")
target_include_directories(lib_domino  PUBLIC
  ${list_include}
)

target_link_libraries(lib_domino PUBLIC
  pthread
)
