cmake_minimum_required(VERSION 2.8)

project(ut)

file(GLOB INC_LIST
  "./src/*"
)
include_directories(${INC_LIST})

file(GLOB SRC_LIST
  "./*.cpp",
  "./src/*/*.cpp"
  "./ut/*/*.cpp"
)
add_executable(ut.exe ${SRC_LIST})

target_link_libraries(ut.exe
  pthread
  gtest
  gtest_main
)
