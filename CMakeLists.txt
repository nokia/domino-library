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

target_link_libraries(ut.exe
  pthread
  gtest
  gmock
)
