cmake_minimum_required(VERSION  3.12)

# ------------------------------------------------------------------------------------------------
project(ut_exe  CXX)  # as early as possible, eg must before CMAKE_CXX_FLAGS

if(COV)
    message("!!! coverage enabled")
    set(CMAKE_CXX_STANDARD  17)  # if gtest fix bug, try c++20/23 to see cov rate inc or not

    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  --coverage")          # gcov req (=-fprofile-arcs -ftest-coverage & -lgcov)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fPIC")               # gcov req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -O0")                 # gcov req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fprofile-abs-path")  # gcc 8

    set(CMAKE_CXX_FLAGS_PROFILE  --coverage)                              # cmake req with -DCMAKE_BUILD_TYPE=PROFILE
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS}  --coverage")  # cmake req before v3.13

    add_compile_definitions(SMART_LOG)
else()
    set(CMAKE_CXX_STANDARD  14)  # low c++ version for wide usage
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -O1")  # fastest build+ut
    add_compile_definitions(WITH_HID_LOG)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED  TRUE)
set(CMAKE_CXX_EXTENSIONS  OFF)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fno-exceptions")  # inc branch coverage

add_executable(ut_exe)

add_subdirectory(src)
add_subdirectory(ut)

target_link_libraries(ut_exe  PRIVATE
  ${LIB_DOMINO_NAME}
  ${LIB_UT_NAME}
  gtest_main
)

# ------------------------------------------------------------------------------------------------
# make run-ut_exe
add_custom_target(run
  COMMAND  ./ut_exe --gtest_brief=0
  DEPENDS  ut_exe
)

# simplify "make help"
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES  true)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES      true)
