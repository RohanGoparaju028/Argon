---
title: Getting started
---

# Getting started

You need Git, CMake 3.16 or newer, and a C++17 compiler. Clang and GCC are the supported native backends. There is no prebuilt compiler distribution documented for this version.

## Build the compiler

```sh
git clone https://github.com/RohanGoparaju028/Argon.git
cd Argon
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
./build/argonc --version
```

The compiler reports `Argon 0.1.0`. CMake builds the compiler itself; your Argon applications use `argonc` directly.

On Windows, run these commands in a MinGW-w64 GCC or Clang environment and use `build/argonc.exe`. Native builds require that compiler and its runtime libraries to be available in the environment.

## Make argonc available anywhere

On macOS or Linux, run this from the cloned repository:

```sh
export PATH="$PWD/build:$PATH"
argonc doctor
```

This updates the current shell. To persist it, add an `export PATH="/absolute/path/to/Argon/build:$PATH"` line with your actual repository path to `~/.zshrc` or your shell's configuration. Alternatively, install with `cmake --install build --prefix /your/prefix` and add `/your/prefix/bin` to PATH. Windows users can add the compiler's build directory to PATH.

## Create a project outside the compiler repository

For example, on macOS or Linux:

```sh
argonc new "$HOME/hello-argon"
cd "$HOME/hello-argon"
argonc run
```

The destination must not already exist. For an existing directory, use `argonc init` from that directory. Both commands refuse to overwrite generated source or manifest files.

The generated project contains:

```text
hello-argon/
  Argon.toml
  .gitignore
  src/
    main.ar
  cpp/
    README.md
```

Replace `src/main.ar` with:

```text
fn add(left: i32, right: i32) -> i32 {
    return left + right;
}

fn main() -> i32 {
    let answer = add(20, 22);
    println(answer);
    return 0;
}
```

Run `argonc run` again. It builds the project and prints `42`. `main` has no parameters and returns `i32`; its return value is the process exit code. Functions are private to their module unless marked `pub`.

## Check, build, and inspect

```sh
argonc check
argonc build
argonc run
argonc build --debug
argonc emit-cpp -o program.cpp
```

`check` validates Argon code and runs global `test` blocks, but does not verify native C++ headers or symbols. `build` creates the executable in `argon-cxx/release/`; debug output goes in `argon-cxx/debug/`. `run` rebuilds before executing. `emit-cpp` lets you inspect the standalone generated C++.

Use `argonc build --cxx-dir generated-cpp` to select another output folder, or set `[project] cxx_dir` in the manifest. Every project has `cpp/` for native headers and sources; [the native guide](cpp.md) explains `cpp "header";` and automatic compilation.

A single file also works without a manifest:

```sh
argonc run /absolute/path/to/example.ar
```

## Try a C++ binding

```sh
argonc new "$HOME/argon-native-demo" --cpp
cd "$HOME/argon-native-demo"
argonc run
```

This scaffolds an Argon caller and a C++ header/source pair. The program prints `42`. See [C++ interoperability](cpp.md) for signatures, linking, and the `unsafe` boundary.

## Test the compiler repository

From your clone of Argon:

```sh
ctest --test-dir build --output-on-failure
./build/argonc run examples/tour.ar
./build/argonc run examples/pointers.ar
./build/argonc run examples/collections.ar
```

CTest discovers additional editor/debug-symbol tests when their tools are installed; the test count can vary by machine. Global `test` blocks in the language are compile-time metaprograms, not a separate runtime test runner.

## Troubleshooting

| Problem | What to check |
| --- | --- |
| `argonc: command not found` | Add its build/install directory to PATH, or use the executable's absolute path. |
| No native C++ compiler found | Run `argonc doctor`; install Clang/GCC or set `ARGON_CXX` to one compiler executable. |
| No manifest found | Run inside the generated project, or pass a source file explicitly. |
| Unknown or private name | Check spelling, imports, and the declaration's `pub` visibility. |
| Native binding fails to compile | Check the header, namespace, exact native signature, and `[cpp]` manifest entries. |
| Empty pointer or bounds error | Check weak upgrades and collection sizes before accessing values. |

Continue with [the language reference](language.md) or [projects and builds](build.md).
