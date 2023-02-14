# make UtLib: so users can call this CMakeLists.txt in their own project
# ------------------------------------------------------------------------------------------------
message("\n*** cmake UtLib ***")

# src
set(LIB_UT_NAME  UtLib  CACHE INTERNAL "")  # global var
file(GLOB  LIB_SRC_LIST
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
)
set_property(SOURCE  ${LIB_SRC_LIST}  PROPERTY COMPILE_FLAGS  ${COMPILE_SRC})
add_library(${LIB_UT_NAME}  OBJECT  ${LIB_SRC_LIST})

# include
file(GLOB  INC_DIR_LIST
  "${CMAKE_CURRENT_SOURCE_DIR}/*"
  "${CMAKE_CURRENT_SOURCE_DIR}/../src/*"
)
list(FILTER  INC_DIR_LIST  EXCLUDE  REGEX  ".*CMakeLists.*")
target_include_directories(${LIB_UT_NAME}  PUBLIC
  ${INC_DIR_LIST}
)

target_link_libraries(${LIB_UT_NAME} PUBLIC
  gtest
  gmock
)
