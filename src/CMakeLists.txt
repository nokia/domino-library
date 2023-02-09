# make DominoLib: so users can call this CMakeLists.txt in their own project
# ------------------------------------------------------------------------------------------------
message("\n*** cmake DominoLib ***")
# src
set(LIB_DOMINO_NAME  DominoLib  CACHE INTERNAL "")  # global var
file(GLOB  LIB_SRC_LIST
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
)
add_library(${LIB_DOMINO_NAME}  ${LIB_SRC_LIST})

# include
file(GLOB  INC_DIR_LIST
  "${CMAKE_CURRENT_SOURCE_DIR}/*"
)
list(FILTER  INC_DIR_LIST  EXCLUDE  REGEX  ".*CMakeLists.*")
target_include_directories(${LIB_DOMINO_NAME}  PUBLIC
  ${INC_DIR_LIST}
)

target_link_libraries(${LIB_DOMINO_NAME} PUBLIC
  pthread
)
