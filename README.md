# `std::pmr` sandbox

**PURPOSE:** Provide a playing ground for `std::pmr` experimentation.

## Description

This repository contains C++ source code that experiments with C++17's
polymorphic memory resources (PMR) facilities. The code here accompanies a talk
titled "C++17's std::pmr Comes With a Cost" by David Sankel that was delivered
at C++Now 2018.

## `std::pmr` shims

At the time of this writing neither libc++ nor libstd++ include an
implementation of `std::pmr` in the `std::pmr` namespace. The header files
`memory_resource.hpp`, `string.hpp`, and `vector.hpp` provide this
functionality. They were pieced together by importing implementations of the
library fundamentals TS into the `std` namespace. There is also an
implementation of `monotonic_buffer_resource` in `memory_resource.hpp` which is
based on Bloomberg's Open Source BDL library.

## Contents

- `libcpp_bug.cpp`. Demonstrate an allocator-related, exception-safety bug that
  is present in both libstdc++ and libc++.
- `simplicity.cpp`. Provide various iterations of a class that is built up to
  allocator awareness. This is the main code used in the presentation.
- `before_after.cpp`. Illustrate a simple class before and after getting
  allocator aware.

## Building

To build, run `make`.

`clang` and `gcc` are used with with `libc++` and `libstdc++` respectively.
Edit the `Makefile` to modify the build process; it is pretty simple. This was
tested with gcc 8.1.0 with libstdc++ and clang 6.0.0 with libc++ 6.0.0.

If you'd like to experiment with the `std::pmr::monotonic_buffer_resource` that
is built off of BDE, build `https://github.com/bloomberg/bde.git` and point the
`Makefile` to its build artifacts.
