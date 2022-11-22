cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")               # gcovr need
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")              # gcovr need
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs")   # gcovr need
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage")  # gcovr need
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")  # inc branch coverage
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")  # gcovr need
endif()

set(UT ut_exe)
project(${UT} CXX)

file(GLOB SRC_LIST  # new file has to "cmake .." again
  "src/*/*.cpp"
  "ut/*/*.cpp"
)
add_executable(${UT} ${SRC_LIST})

file(GLOB INC_DIR_LIST
  "src/*"
  "ut/*"
)
target_include_directories(${UT} PRIVATE ${INC_DIR_LIST})

target_link_libraries(${UT} PRIVATE
  gtest
  gtest_main
  gmock
  pthread
)

set(RUN_UT run-${UT})
add_custom_target(${RUN_UT} COMMAND ./${UT} --gtest_brief=1 DEPENDS ${UT})  # make run-ut_exe
