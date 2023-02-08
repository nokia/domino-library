message("\n*** cmake DominoLib ***")

# make DominoLib
# ------------------------------------------------------------------------------------------------
file(GLOB  LIB_SRC_LIST
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
)
set(LIB_DOMINO_NAME  DominoLib  CACHE INTERNAL "")  # global var
add_library(${LIB_DOMINO_NAME}  ${LIB_SRC_LIST})

file(GLOB  INC_DIR_LIST
  "${CMAKE_CURRENT_SOURCE_DIR}/*"
)
list(FILTER  INC_DIR_LIST  EXCLUDE  REGEX  ".*CMakeLists.*")
target_include_directories(${LIB_DOMINO_NAME}
  PUBLIC
  ${INC_DIR_LIST}
)

target_link_libraries(${LIB_DOMINO_NAME} PUBLIC
  pthread
)
