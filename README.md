# Domino Library

A small C++17 library.

Talk is cheap. Use $\color{red}{\textsf{AI}}$ to show you the lib:
- What
- How
- Quality: Opus4.5 scores this lib=92% (vs it's self-gen=68%).

[![Build Status](https://github.com/nokia/domino-library/actions/workflows/ci.yml/badge.svg)](https://github.com/nokia/domino-library/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/nokia/domino-library/branch/main/graph/badge.svg?token=LGK8GD9GJD)](https://codecov.io/gh/nokia/domino-library)
[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](LICENSE)

![Domino tiles](image/domino.jpg)

## Components

- **Domino**
  - **Problem:** such as OS-upgrade, involves many interdependent conditions & tasks.
  - Each condition is a domino tile, and their dependencies form a DAG.
  - A condition satisfied = a tile falls → auto-broadcasts to downstream (domino).
  - A task can stick to 1 tile - when the tile falls, auto-execute the task.
  - **Result:** improved eNB-base-station-upgrade $\color{blue}{\textsf{from ~9min to ~1min}}$.

- **ObjAnywhere**

- **MsgSelf** is a priority FIFO queue...

- **SafePtr** is safer than std::shared_ptr

- **SmartLog**

- **ThreadBack:** time-cost event running in a separate thread → callback main thread

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
