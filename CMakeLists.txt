cmake_minimum_required(VERSION  3.14)

# ------------------------------------------------------------------------------------------------
project(ut_exe  CXX)  # as early as possible, eg must before CMAKE_CXX_FLAGS

set(CMAKE_CXX_EXTENSIONS  OFF)

if(ci  STREQUAL  "cov")
    message("!!! coverage & valgrind(only need -g -fno-omit-frame-pointer)")
    add_compile_options(--coverage -fPIC -O0 -fprofile-abs-path -g)  # gcov + valgrind req
    add_link_options(--coverage)                                     # gcov req
    add_compile_definitions(SMART_LOG WITH_HID_LOG)  # more info to debug ci

elseif(ci  STREQUAL  "asan")
    message("!!! asan+lsan+ubsan for: use-after-free, double-free, out-of-bound, leak, undef behavior")
    add_compile_definitions(_FORTIFY_SOURCE=2)     # buffer overflow checks
    add_compile_options(-g -fno-omit-frame-pointer)
    add_compile_options(-fstack-protector-strong)  # stack overflow protection
    add_compile_options(-O2)  # required for FORTIFY_SOURCE
    add_compile_options(-fsanitize=address,undefined)

    add_link_options(-fsanitize=address,undefined)
    add_link_options(-fstack-protector-strong)     # ensure stack protector runtime is linked

elseif(ci  STREQUAL  "tsan")  # asan/lsan/msan cover by valgrind(memcheck)
    message("!!! SANITIZE for: thread")
    add_compile_definitions(_FORTIFY_SOURCE=2)     # buffer overflow checks
    add_compile_options(-g -fno-omit-frame-pointer)
    add_compile_options(-fstack-protector-strong)  # stack overflow protection
    add_compile_options(-O2)  # required for FORTIFY_SOURCE
    add_compile_options(-fsanitize=thread)

    add_link_options(-fsanitize=thread)
    add_link_options(-fstack-protector-strong)     # ensure stack protector runtime is linked

else()
    message("!!! UT only - no cov, no SANITIZE check etc")
    add_compile_options(-O1)  # fastest build+ut (1-cpp-change=5s, vs 45s of -g)
endif()

#add_compile_options(-fno-exceptions)  # inc branch coverage

add_executable(ut_exe)

# googletest via FetchContent (always latest, shallow clone to min download)
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY  https://github.com/google/googletest.git
    GIT_TAG         main
    GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(src)
add_subdirectory(ut)

# C++ standard on targets
if(ci  STREQUAL  "cov")
    target_compile_features(lib_domino  PUBLIC  cxx_std_20)  # try c++20/23 to see cov rate inc or not
else()
    target_compile_features(lib_domino  PUBLIC  cxx_std_17)  # 17 since "if constexpr()"
endif()

target_link_libraries(ut_exe  PRIVATE
    lib_domino
    lib_ut
    gtest_main
)

# make run
add_custom_target(run
    COMMAND  ./ut_exe --gtest_brief=0
    DEPENDS  ut_exe
)

# simplify "make help"
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES  true)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES      true)
