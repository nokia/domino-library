cmake_minimum_required(VERSION 2.8...3.23)

project(ut)

file(GLOB INC_DIR_LIST
  "./src/*"
  "./ut/*"
)
include_directories(${INC_DIR_LIST})

file(GLOB SRC_LIST
  "./src/*/*.cpp"
  "./ut/*/*.cpp"
)
add_executable(ut.exe ${SRC_LIST})

if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")               # gcovr need
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")              # gcovr need
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs")   # gcovr need
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage")  # gcovr need
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")  # inc branch coverage
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")  # gcovr need
endif()

target_link_libraries(ut.exe
  pthread
  gtest
  gmock
)
