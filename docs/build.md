---
title: Projects and builds
---

# Argon Build

`argonc` owns the application build workflow. There is no separate Makefile, CMake project, or executable build script for an Argon application. CMake is used only to build the compiler itself.

```sh
argonc new hello
cd hello
argonc check
argonc build
argonc run
```

`new` requires a destination that does not exist. `init [DIRECTORY]` works in an existing directory, refusing to overwrite generated source or manifest paths. Existing `.gitignore` files are preserved; add `.argon/` yourself if one already exists. `--cpp` works with both creation commands and adds a complete local C++ library example. Directory paths containing spaces are supported.

## Manifest

```toml
[project]
schema = 1
name = "hello"
entry = "src/main.ar"

[cpp]
headers = ["cmath", "demo.hpp"]
sources = ["native/demo.cpp"]
include_dirs = ["native"]
library_dirs = []
libraries = []
```

The file is `Argon.toml`. The parser supports this deliberately small TOML subset: `[project]` and `[cpp]` sections, integer schema `1`, double-quoted strings, single-line arrays of strings, and `#` comments outside strings. Other TOML features are not accepted. Unknown sections/keys and duplicate keys are errors.

`name` must contain 1–64 ASCII letters, digits, underscores, or hyphens and start with a letter or digit. `entry` defaults to `src/main.ar` when omitted. All manifest file/directory paths must be relative, must exist, and must resolve inside the project; absolute paths and escaping symlinks are rejected. The `[cpp]` section is optional. Native headers are included by name, native sources are compiled with the application, and libraries are linked in their listed order.

Imports define the Argon source dependency graph. You do not maintain a list of Argon source files. Each canonical module is parsed once. Existing module search rules can reach files outside the project; manifest validation is not a sandbox for source imports.

## Commands and output

| Command | Behavior |
| --- | --- |
| `argonc check [FILE]` | Type-check and execute global test blocks; no external compiler required |
| `argonc build [FILE]` | Build `.argon/build/release/NAME` |
| `argonc run [FILE]` | Build and execute, with the project root as working directory |
| `argonc build --debug` | Build `.argon/build/debug/NAME` with `-O0 -g` |
| `argonc run --debug` | Build and execute the debug profile |
| `argonc build -o PATH` | Select an executable path; its parent must exist |
| `argonc emit-cpp [FILE] -o FILE.cpp` | Write standalone generated C++; omit `-o` to use stdout |
| `argonc FILE.ar [-o PATH]` | Single-file shorthand; default executable is beside the current invocation |

Windows executables have an `.exe` suffix for default output paths. With no FILE, commands locate the nearest manifest by walking upward from the current directory. With a FILE, they look for a manifest starting at that file's directory; otherwise a standalone project uses its containing directory. `.fe` entry files are also accepted.

The builder uses a private temporary staging directory, then atomically replaces the executable after a successful native build. Failed compile-time tests, type checks, or C++ compilation preserve the previous executable and prevent `run` from executing it. Output symlinks and hard-link aliases are rejected. Successful builds retain standalone `program.cpp` in the profile directory for inspection. On macOS, debug builds also retain the matching `.dSYM` bundle beside the executable, including for custom `-o` paths. Keep that bundle with the executable when debugging.

Application compilation uses Clang/GCC with C++17, `-O2` for release and `-O0 -g` for debug, and no fast-math transformations. `ARGON_CXX` selects one compiler executable path or name, never a shell command. Otherwise discovery searches `clang++`, `g++`, then `c++`. Compiler arguments are passed directly to the process without a shell.

## Current boundaries

Builds always recheck and rebuild. There is no remote registry, package downloading, dependency version resolver, incremental cache, custom shell hooks, cross-compilation interface, or automatic compiler installation. Local C++ sources and preinstalled native libraries are supported. A C++ compiler is needed for native builds, but users do not configure a separate application build system.

Compile-time computations are bounded and deterministic for fixed inputs and target behavior. Generated C++ is stable for unchanged sources at the same paths. Executable bytes, debug paths, and floating-point results are not promised reproducible across operating systems or compiler versions.

## Editors

[VS Code](vscode.md) provides file/project runners, checks, and CodeLLDB debugging. The shared [Vim/Neovim plugin](vim.md) provides the same commands, quickfix checks, and LLDB or optional nvim-dap debugging. All use `argonc` and its native source-mapped debug builds.

## Export and the migration goal

For an application without external native dependencies:

```sh
argonc emit-cpp -o program.cpp
c++ -std=c++17 program.cpp -o program
./program
```

The generated file includes runtime support and can compile without `argonc`. Applications with C++ bindings still require their native headers, sources, include paths, and libraries to be supplied to the C++ compiler.

The intended future migration path is an idiomatic, maintainable C++ project with meaningful names, organized source files, runtime support where required, and a standalone build configuration. **That migration export is not implemented.** Today's output is standalone generated compiler code. Preserving Argon's checked operations and reference-counted behavior is part of the migration goal.
