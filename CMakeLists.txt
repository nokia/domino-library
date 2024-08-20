cmake_minimum_required(VERSION  3.12)

# ------------------------------------------------------------------------------------------------
project(ut_exe  CXX)  # as early as possible, eg must before CMAKE_CXX_FLAGS

if(ci  STREQUAL  "cov")
    message("!!! coverage & valgrind(only need -g -fno-omit-frame-pointer)")
    set(CMAKE_CXX_STANDARD  20)  # if gtest fix bug, try c++20/23 to see cov rate inc or not

    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  --coverage")          # gcov req (=-fprofile-arcs -ftest-coverage & -lgcov)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fPIC")               # gcov req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -O0")                 # gcov req
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fprofile-abs-path")  # gcc 8

    add_compile_definitions(SMART_LOG)
    add_compile_definitions(WITH_HID_LOG)  # more info to debug ci

elseif(ci  STREQUAL  "tsan")  # asan/lsan/msan cover by valgrind(memcheck)
    set(CMAKE_CXX_STANDARD  17)
    message("!!! SANITIZE for: thread")
    set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS}  -g -fno-omit-frame-pointer -fsanitize=thread")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS}  -fsanitize=thread")

elseif(ci  STREQUAL  "ubsan")
    set(CMAKE_CXX_STANDARD  17)
    message("!!! SANITIZE for: undefined behavior")
    set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS}  -g -fno-omit-frame-pointer -fsanitize=undefined")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS}  -fsanitize=undefined")

else()
    message("!!! UT only - no cov, no SANITIZE check etc")
    set(CMAKE_CXX_STANDARD  17)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -O1")  # fastest build+ut (1-cpp-change=5s, vs 45s of -g)
    #add_compile_definitions(WITH_HID_LOG)  # cov no HID; easy to turn-on
endif()

set(CMAKE_CXX_STANDARD_REQUIRED  TRUE)
set(CMAKE_CXX_EXTENSIONS  OFF)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fno-exceptions")  # inc branch coverage

add_executable(ut_exe)

set(gtest_dir ${CMAKE_CURRENT_SOURCE_DIR}/../googletest)
add_subdirectory(${gtest_dir}
    ${CMAKE_BINARY_DIR}/googletest)  # must specify where to build since gtest out domino root
add_subdirectory(src)
add_subdirectory(ut)

target_link_libraries(ut_exe  PRIVATE
    lib_domino
    lib_ut
    gtest_main
)

# make run-ut_exe
add_custom_target(run
    COMMAND  ./ut_exe --gtest_brief=0
    DEPENDS  ut_exe
)

# simplify "make help"
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES  true)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES      true)
