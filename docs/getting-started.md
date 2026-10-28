---
title: Getting started
---

# Getting started

## Install the compiler first

Follow the [installation guide](installation.md) for your operating system:

- [macOS](installation.md#macos)
- [Linux: Ubuntu/Debian, Fedora, or Arch](installation.md#linux)
- [Windows: MSYS2 UCRT64](installation.md#windows)

It covers prerequisites, building from source, installing `argonc`, and permanent PATH setup. Then verify in your configured terminal:

```sh
argonc --version
argonc doctor
```

The current compiler reports `Argon 0.1.0`. Keep the C++ compiler installed for native application builds. The commands below use macOS/Linux shell syntax, which also works in the Windows UCRT64 Bash terminal from the installation guide.

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
cd build-install
ctest --output-on-failure
cd ..
argonc run examples/tour.ar
argonc run examples/pointers.ar
argonc run examples/collections.ar
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
