cmake_minimum_required(VERSION  3.8)

# ------------------------------------------------------------------------------------------------
set(UT  ut_exe)
project(${UT}  CXX)  # as early as possible, eg must before CMAKE_CXX_FLAGS

set(CMAKE_CXX_STANDARD  14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS  OFF)
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -O0")                 # gcovr req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fprofile-arcs")      # gcovr req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -ftest-coverage")     # gcovr req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fPIC")               # gcovr opt
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fprofile-abs-path")  # gcovr opt

    set(CMAKE_CXX_FLAGS_PROFILE  --coverage)                              # gcovr req
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS}  --coverage")  # gcovr req

    add_compile_definitions(SMART_LOG)
else()
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -O1")  # gcovr req
    add_compile_definitions(WITH_HID_LOG)
endif()
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fno-exceptions")  # inc branch coverage

file(WRITE  ${CMAKE_CURRENT_BINARY_DIR}/null.cpp  "")
add_executable(${UT}  "${CMAKE_CURRENT_BINARY_DIR}/null.cpp")  # cmake cannot w/o cpp

add_subdirectory(src)
add_subdirectory(ut)

target_link_libraries(${UT}  PRIVATE
  ${LIB_DOMINO_NAME}
  ${LIB_UT_NAME}
  gtest_main
)

# ------------------------------------------------------------------------------------------------
# make run-ut_exe
set(RUN_UT  run)
add_custom_target(${RUN_UT}
  COMMAND  ./${UT} --gtest_brief=0
  DEPENDS  ${UT}
)

# simplify "make help"
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES  true)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES      true)
