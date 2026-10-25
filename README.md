# Argon

**A small subset of c++ for projects that need C++  without necessarily need c++  files.**

Argon has its own syntax and reference-counted heap objects and collections, checked arithmetic, and explicit one-way C++ bindings. `argonc` is both the compiler and the project's build tool. The compiler is written entirely in C++17.
 It is a restricted language with C++ interoperability, rather than a literal syntactic subset of C++.

## Build the compiler

You need CMake 3.16+ and a C++17 compiler. Clang and GCC are the supported native backends.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/argonc --version
```

On Windows, use a MinGW-w64 GCC or Clang environment and `build/argonc.exe`. To install, run `cmake --install build --prefix /your/prefix` and put that prefix's `bin` directory on PATH. The runtime is embedded in `argonc`; no separate runtime source installation is needed.

## Start a project

With `argonc` on PATH:

```sh
argonc new hello
cd hello
argonc run
```

The generated project contains `Argon.toml`, `.gitignore`, and `src/main.ar`:

```rust
test {
    const answer: i32 = 6 * 7;
    assert(answer == 42);
}

fn main() -> i32 {
    println("Hello from Argon!");
    println(answer);
    return 0;
}
```

`test` is Argon's metaprogramming keyword. Its block lives at global scope and executes during compilation, including `argonc check`. Direct `const` declarations in the block become module constants; temporary `let` bindings stay in the block. A failed assertion stops compilation. No extra `meta` or `pure` keywords are used.

| Command | Purpose |
| --- | --- |
| `argonc new NAME` | Create a working project |
| `argonc new NAME --cpp` | Create a project with a real C++ library binding |
| `argonc init [DIRECTORY]` | Initialize an existing directory without overwriting source files |
| `argonc check` | Resolve imports, check types, and execute compile-time blocks |
| `argonc build` | Build a native executable |
| `argonc run` | Build and execute the project, returning its exit status |
| `argonc build --debug` | Build with debug information |
| `argonc emit-cpp -o program.cpp` | Export standalone generated C++ |
| `argonc doctor` | Locate the native compiler |

Project commands discover the nearest `Argon.toml`, including from a source subdirectory. A single file also works: `argonc run examples/tour.ar`, `argonc check file.ar`, or `argonc file.ar -o app`. `.ar` is the new extension; `.fe` is also accepted for familiar source files. Existing source must use the features supported by this new implementation.

## C++ libraries

```sh
argonc new native-demo --cpp
cd native-demo
argonc run
```

That command scaffolds an Argon application, a C++ header and source, and the native build configuration. The application calls a C++ function and prints `42`:

```rust
extern cpp fn twice(value: i32) -> i32 = "demo::twice";

fn main() -> i32 {
    unsafe {
        println(twice(21));
    }
    return 0;
}
```

Bindings currently support numeric and boolean values plus `void` returns. The generated bridge checks the exact C++ function signature during native compilation. C++ calls require `unsafe` because arbitrary native libraries can violate memory safety. See [C++ interoperability](docs/cpp.md).

## Implemented language

- Typed functions, inferred local bindings, immutable constants, explicit returns, and scalar casts.
- `i32`, `i64`, `f64`, `bool`, and byte strings; checked integer overflow, numeric narrowing, division, and finite floating-point results.
- Value structs, reference-counted classes, initialized fields, and deterministic cleanup when the last owner is released.
- Reference-counted `Array<T>`, `LinkedList<T>`, `Deque<T>`, `Stack<T>`, `Queue<T>`, `HashMap<K,V>`, and `HashSet<T>` with checked access and explicit storage cloning.
- Typed owning `Ptr<T>` and non-owning `Weak<T>` handles, checked dereferencing, weak upgrade, expiration tests, and owner counts.
- VS Code run/check/build commands and CodeLLDB debugging; matching Vim/Neovim commands with LLDB and optional nvim-dap.
- Tagged enums with payloads and exhaustive `match` expressions.
- `switch` statements with literal cases and no implicit fallthrough.
- `if`, `while`, C-style `for`, `break`, `continue`, and unary function pipelines with `|>`.
- Local modules through `import`, with explicit `pub` exports and private declarations by default.
- Global `test` blocks with bounded, deterministic scalar evaluation, ordinary function calls, generated constants, and compile-time assertions.
- An integrated build tool with local C++ sources, headers, include paths, library directories, and system-library linking.

## Determinism and safety

Compile-time blocks cannot perform I/O or call C++; they have no filesystem, network, clock, randomness, or environment APIs. They run with fixed instruction and recursion limits. Their results depend on explicit inputs and source constants. Ordinary application code may perform I/O, so whole-program determinism is not promised. Floating-point results across different compiler targets are not guaranteed identical.

Classes, owning pointers, and collection storage use automatic reference counting. Copying a class, owning pointer, or collection retains the same allocation; structs keep value semantics and retain any owning fields they contain. Weak pointers observe allocations without keeping them alive. Strong cycles are not collected: use weak back-references. The language exposes no raw addresses or escaping native references and checks pointer/collection access and integer operations. These protections are not a formal memory-safety proof, and foreign C++ code remains outside them. See [memory and collections](docs/memory.md).

This first implementation does **not** include inheritance, instance methods, closures, async, borrowing, user-defined generics, arbitrary C++ header import, package downloads, incremental compilation, or general syntax-generating macros. Metaprogramming currently computes scalar constants and validates programs through `test` blocks. See [the language reference](docs/language.md).

## Why Argon exists

Argon's focus is the small greenfield application that needs a native C++ library. Its goal is to make project creation, safe application code, compile-time computation, and native linking work together through one command. That is the project's intended distinction from broader C++ successor efforts such as Carbon; it is not a claim that those projects cannot address similar use cases.

- [Language syntax and semantics](docs/language.md)
- [Argon Build and the manifest](docs/build.md)
- [C++ bindings and safety boundaries](docs/cpp.md)
- [Memory and collections](docs/memory.md)
- [VS Code runner and debugger](editors/vscode/README.md)
- [Vim and Neovim support](editors/vim/README.md)
- [Runnable examples](examples/README.md)

Licensed under Apache-2.0.
