# C++ interoperability

Argon consumes C++ libraries through explicit function bindings. The compiler and builder are written in C++17, and Argon programs lower to checked C++17 and native machine code.

## A working native project

```sh
argonc new native-demo --cpp
cd native-demo
argonc run
```

This produces `src/main.ar`, `native/demo.hpp`, `native/demo.cpp`, and `Argon.toml`. The builder compiles both languages and links the result in one command.

The header declares an exact native signature:

```cpp
#pragma once
#include <cstdint>
namespace demo {
    std::int32_t twice(std::int32_t value);
}
```

The binding uses the existing Argon function syntax plus an explicit C++ target:

```rust
extern cpp fn twice(value: i32) -> i32 = "demo::twice";

fn main() -> i32 {
    unsafe {
        println(twice(21));
    }
    return 0;
}
```

```toml
[project]
schema = 1
name = "native-demo"
entry = "src/main.ar"

[cpp]
headers = ["demo.hpp"]
sources = ["native/demo.cpp"]
include_dirs = ["native"]
```

The bridge takes a typed function pointer to `demo::twice`. Native compilation fails if the header does not expose an exact compatible function signature; the bridge does not silently narrow an unrelated overload. Use `std::int32_t` for `i32`, `std::int64_t` for `i64`, `double` for `f64`, and `bool` for `bool`.

## An existing standard-library API

Set `[cpp] headers = ["cmath"]` and declare:

```rust
extern cpp fn square_root(value: f64) -> f64 = "std::sqrt";

fn main() -> i32 {
    unsafe { println(square_root(81.0)); }
    return 0;
}
```

For installed libraries, add header names, project-local include/library directories when necessary, and `libraries = ["name"]`. This produces the compiler's ordinary `-I`, `-L`, and `-l` arguments. A library with a complex API should expose a small C++ wrapper with these supported scalar signatures. Native dependency installation is not provided yet.

## Safety boundary

Numeric and boolean arguments/results and `void` results are supported. Raw pointers, references, strings across the boundary, native classes, callbacks, templates as binding targets, variadic functions, and ownership-bearing handles are not supported. Wrappers can use arbitrary C++ internally and present a simple scalar interface.

Calling a foreign function requires a lexical `unsafe` block. `argonc check` validates the declared Argon signature and placement; the actual C++ signature and header availability are verified during native compilation. Foreign calls and unsafe blocks cannot execute in global `test` metaprograms, including through helper functions.

The language cannot make arbitrary C++ implementations memory-safe. Incorrect wrappers, memory corruption, native undefined behavior, and crashes remain possible inside libraries. Returning a non-finite floating-point value produces a checked Argon runtime error. Standard C++ exceptions reaching the executable entry are reported as runtime errors; non-standard exceptions and native crashes are not converted into Argon errors. There is no guarantee that an unsafe wrapper function behaves safely merely because an application calls it through a normal Argon function.
