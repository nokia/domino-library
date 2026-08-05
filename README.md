# Domino Library

A small C++17 library.

"Talk is cheap. Show me the $\color{red}{\textsf{code}}$." $\color{red}{\textsf{AI}}$ can summarize the lib:
- What
- How
- Quality: Opus4.5 scores this lib=92% (vs it's self-gen=68%).

[![Build Status](https://github.com/nokia/domino-library/actions/workflows/ci.yml/badge.svg)](https://github.com/nokia/domino-library/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/nokia/domino-library/branch/main/graph/badge.svg?token=LGK8GD9GJD)](https://codecov.io/gh/nokia/domino-library)
[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](LICENSE)

![Domino tiles](image/domino.jpg)

## Components

- **Domino**
  - **Problem:** Coordinate workflows such as software upgrades, where many
    conditions and tasks depend on one another.
  - **Model:** Each condition is an event (a domino tile), each task is a
    handler, and their dependencies form a DAG.
  - **Execution:** When an event occurs, downstream states are deduced and newly
    satisfied handlers run, propagating through the graph like falling dominoes.
  - **Trigger rule:** A handler reacts only when its event changes from false to
    true.
  - [source](src/domino/Domino.hpp) · [tests](ut/domino/DominoTest.cpp) · [中文手册](https://mp.weixin.qq.com/s/ckF2LXH4hDcIYbZNqSIb0g)

- **MsgSelf** is a priority FIFO queue that defers callbacks until the current
  call stack has returned to the main loop.
  ([source](src/msg_self/MsgSelf.hpp) · [tests](ut/msg_self/MsgSelfTest.cpp) · [中文手册](https://mp.weixin.qq.com/s/aPjhY7nRmlD4xHhUL_ykxg))

- **ThreadBack** runs time-consuming work in background threads and lets the
  main thread collect results and invoke completion callbacks.
  ([source](src/thread/ThreadBack.hpp) · [tests](ut/thread) · [中文手册](https://mp.weixin.qq.com/s/bb1slMqhuoBLZZCd3NmbYA))

- **SmartLog** retains diagnostic context but emits it only when needed—for
  example, to print detailed logs only for failed test cases.
  ([source](src/log/UniSmartLog.hpp) · [tests](ut/log/UniSmartLogTest.cpp) · [中文手册](https://mp.weixin.qq.com/s/KNKBC-uHOylRXxpspZbVnA) · [English manual](https://mp.weixin.qq.com/s/X3XZOGOQGDQtwQDEPNA32A) · [sample output](image/ut_smartlog.jpg))

- **ObjAnywhere** is a process-local object registry for cases where explicitly
  passing the same shared service through many call layers is impractical.
  ([source](src/obj_anywhere/ObjAnywhere.hpp) · [tests](ut/obj_anywhere/ObjAnywhereTest.cpp) · [中文手册](https://mp.weixin.qq.com/s/SYE3xkz-Zqp-l46ZpjnKWg))

- **SafePtr** restricts pointer creation and cross-type conversion around
  `std::shared_ptr`, rejecting or nulling unsafe conversions.
  ([source](src/safe_mem/SafePtr.hpp) · [tests](ut/safe_mem/SafePtrTest.cpp))

The components are small and composable. See
[UtInitObjAnywhere.hpp](ut/obj_anywhere/UtInitObjAnywhere.hpp) for an integration
example.

## Engineering Evidence

- [GitHub Actions](https://github.com/nokia/domino-library/actions/workflows/ci.yml)
  runs GoogleTest under Valgrind on pushes, pull requests, and a weekly schedule.
- [Coverage gates](ut/gcovr/gcovr.cfg) require at least **97.8% line coverage**
  and **89.8% branch coverage**.
- The tests under [`ut/`](ut) are executable examples covering normal behavior,
  edge cases, and failure paths.
- The currently verified CI environment is **Ubuntu + GCC**.

## Build and Run

Requirements: CMake 3.14 or newer and a C++17-capable compiler.

```bash
git clone https://github.com/nokia/domino-library.git
cd domino-library
cmake -S . -B build
cmake --build build -j
cmake --build build --target run
```

For source integration with CMake:

```cmake
add_subdirectory(third_party/domino-library/src)
target_link_libraries(your_target PRIVATE lib_domino)
```

The source can also be copied into another project under the terms of the
BSD-3-Clause license.

## Design Boundaries

- Domino dependency graphs must remain acyclic.
- Domino, MsgSelf, ObjAnywhere, and SmartLog are intended for single-thread or
  main-thread use; they are not synchronization primitives.
- ThreadBack invokes completion callbacks only when the main thread collects
  completed work.
- SafePtr narrows unsafe ownership and casting operations, but it does not make
  the pointed-to object thread-safe or prevent circular ownership.

## Contributing

Issues and pull requests are welcome. Changes should include or update unit
tests; the existing tests are the primary usage examples.

## Maintainer

Primary maintainer: [fchn289](https://github.com/fchn289)  
Contribution history: [GitHub contributors](https://github.com/nokia/domino-library/graphs/contributors)  
Contact: sz.chen@nokia-sbell.com

## License

[BSD 3-Clause License](LICENSE)
