---
title: C++ interoperability
---

# Use native headers and source files

Argon projects include a visible `cpp/` directory. Copy a small library's headers and implementation files there, declare its header with `cpp "header";`, and expose the functions you need with `extern cpp fn`.

## A complete working project

```sh
argonc new native-demo --cpp
cd native-demo
argonc run
```

This prints `42`. It creates:

```text
native-demo/
  Argon.toml
  src/main.ar
  cpp/
    README.md
    demo.hpp
    demo.cpp
  argon-cxx/release/       # Created by argonc run.
    program.cpp
    native-demo
```

`cpp/demo.hpp` declares the function:

```cpp
#pragma once
#include <cstdint>
namespace demo {
    std::int32_t twice(std::int32_t value);
}
```

`cpp/demo.cpp` implements it:

```cpp
#include "demo.hpp"
std::int32_t demo::twice(std::int32_t value) {
    return value * 2;
}
```

`src/main.ar` includes the header and binds the function:

```text
cpp "demo.hpp";

extern cpp fn twice(value: i32) -> i32 = "demo::twice";

fn main() -> i32 {
    unsafe { println(twice(21)); }
    return 0;
}
```

No native file list is required for files in `cpp/`. The manifest can be just:

```toml
[project]
schema = 1
name = "native-demo"
entry = "src/main.ar"
cxx_dir = "argon-cxx"
```

## One quoted syntax for every header

```text
cpp "demo.hpp";
cpp "vendor/math.hpp";
cpp "cmath";
```

Declarations belong at module scope. They can appear before/interspersed with the initial import list or among top-level declarations. Header lookup searches `cpp/` first, additional manifest include directories next, and then the native compiler's standard include paths. There is no angle-bracket form in Argon. Imported Argon modules contribute their header declarations too.

Header names must be relative include names; absolute paths, parent traversal, and invalid characters are rejected. Inclusion is verified by the native compiler during `build`/`run`, not by `check`. Builtins such as `println`, `length`, and `assert` need no header declarations.

## Standard-library functions

No local `cmath` file is necessary:

```text
cpp "cmath";
extern cpp fn square_root(value: f64) -> f64 = "std::sqrt";

fn main() -> i32 {
    unsafe { println(square_root(81.0)); }
    return 0;
}
```

This prints `9`. The bridge selects the exact overload matching the declared signature. Use `std::int32_t` for `i32`, `std::int64_t` for `i64`, `double` for `f64`, and `bool` for `bool`.

## Header-only libraries

Place this in `cpp/inline_math.hpp`:

```cpp
#pragma once
#include <cstdint>
inline std::int32_t triple(std::int32_t value) {
    return value * 3;
}
```

Then use:

```text
cpp "inline_math.hpp";
extern cpp fn triple(value: i32) -> i32 = "triple";

fn main() -> i32 {
    unsafe { println(triple(14)); }
    return 0;
}
```

No `.cpp` file is needed. Header-only implementations are compiled as part of the generated translation unit. Libraries with templates, macros, or complex argument types may still need a wrapper exposing a supported function signature.

## Copied C and C++ implementations

The builder recursively discovers `.cpp`, `.cc`, `.cxx`, `.c++`, and `.c` files under `cpp/`. C++ sources compile as C++17. C sources compile separately as C11, then their objects are linked with the application. Preserve the library's directory layout so its relative includes still work.

C headers need C linkage when consumed from C++:

```c
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t answer(void);
#ifdef __cplusplus
}
#endif
```

The `.c` implementation can define `int32_t answer(void) { return 42; }`. In Argon, use `cpp "answer.h";` and `extern cpp fn answer() -> i32 = "answer";`.

Copy only the required implementation files. Every discovered source is compiled; library examples/tests with their own `main` or mutually exclusive platform implementations must be kept outside `cpp/`. The builder does not infer a library's configuration macros or specialized build requirements.

## Additional include paths and installed libraries

Use `[cpp]` when the library needs more than copied files:

```toml
[cpp]
headers = []
sources = ["vendor/extra.cpp"]
include_dirs = ["vendor/include"]
library_dirs = ["vendor/lib"]
libraries = ["example"]
```

These input paths must exist and stay within the project. The header/source arrays remain supported for existing projects. Sources listed explicitly and also discovered under `cpp/` compile once. Library names generate ordinary `-l` options; include/library paths generate `-I`/`-L` options. Native dependency installation is not provided.

## Choose where generated C++ goes

```sh
argonc build --cxx-dir generated-cpp
argonc run --cxx-dir generated-cpp
argonc emit-cpp --cxx-dir generated-cpp
```

These write `generated-cpp/release/program.cpp`; builds also create the native executable there by default. For a persistent project setting, use `argonc new app --cxx-dir generated-cpp` or edit `[project] cxx_dir`. The default is `argon-cxx`, and debug builds use its `debug/` subdirectory. See [the build reference](build.md).

## What including a header does not do

The compiler currently binds functions with numeric/Boolean arguments and numeric/Boolean/void results. Including a header does not automatically import C++ classes, strings, references, pointers, templates, callbacks, or overload sets into Argon. For example, including a CPR header alone does not make CPR's request/response objects usable in Argon. Such APIs still need compatible wrappers; full object/string interoperability is a separate capability.

Foreign calls require a lexical `unsafe` block and cannot run in compile-time `test` blocks. Arbitrary C++ code can corrupt memory or crash. Non-finite floating-point results become checked Argon errors; standard C++ exceptions reaching the program entry are reported, but native crashes and non-standard exceptions are not converted.

Run the repository's [native-files example](https://github.com/RohanGoparaju028/Argon/tree/main/examples/native_files) to exercise local, header-only, C, and standard-library functionality together.
