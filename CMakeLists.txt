cmake_minimum_required(VERSION  3.14)

set(UT  ut_exe)
project(${UT}  CXX)  # as early as possible, eg must before CMAKE_CXX_FLAGS

set(CMAKE_CXX_STANDARD  14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS  OFF)

if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -O0")              # gcovr req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fprofile-arcs")   # gcovr req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -ftest-coverage")  # gcovr req
    set(CMAKE_CXX_FLAGS_PROFILE  --coverage)                     # gcovr req
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS}  --coverage")  # gcovr req

    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -g")     # gcovr opt
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fPIC")  # gcovr opt
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fprofile-abs-path")  # gcovr opt

    add_compile_definitions(SMART_LOG)
else()
    add_compile_definitions(WITH_HID_LOG)
endif()
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fno-exceptions")  # inc branch coverage

file(GLOB SRC_LIST  # new file has to "cmake .." again
  "src/*/*.cpp"
  "ut/*/*.cpp"
)
add_executable(${UT}  ${SRC_LIST})

file(GLOB  INC_DIR_LIST
  "src/*"
  "ut/*"
)
target_include_directories(${UT}  PRIVATE  ${INC_DIR_LIST})

target_link_libraries(${UT}  PRIVATE
  gtest
  gtest_main
  gmock
  pthread
)

# make run-ut_exe
set(RUN_UT  run-${UT})
add_custom_target(${RUN_UT}
  COMMAND  ./${UT} --gtest_brief=0
  DEPENDS  ${UT}
)

# simplify "make help"
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES  true)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES      true)
