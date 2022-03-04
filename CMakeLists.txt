cmake_minimum_required(VERSION 2.8)

project(ut)

include_directories(./src/*)

file(GLOB SRC_LIST
  "./*.cpp",
  "./src/*/*.cpp"
  "./ut/*/*.cpp"
)
add_executable(ut ${SRC_LIST})

target_link_libraries(ut
  pthread
  gtest
  gtest_main
)
