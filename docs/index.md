---
title: Overview
---

# Small native projects. One command to build.

Argon is an experimental programming language for small-to-medium projects that need C++ libraries without managing a separate C++ build system. It combines its own typed syntax, checked operations, reference-counted heap storage, and an integrated compiler and project builder: **`argonc`**. The compiler is written in C++17 and emits C++17 for a native compiler to build.

**These docs describe version 0.1.0, the current prototype.** The language and tools are still evolving. Read the [current status and limitations](status.md) before choosing it for a project.

## Start here

See **[what Argon supports today](features.md)** for the feature inventory, or **[the complete keyword reference](keywords.md)** for every implemented keyword, its rules, and runnable examples.

1. [Install on macOS, Linux, or Windows](installation.md), then [create your first project](getting-started.md).
2. [Learn the keywords](keywords.md), then consult the [language reference](language.md) for detailed semantics.
3. [Understand reference counting](memory.md), owning pointers, weak pointers, and collections.
4. [Call a C++ function](cpp.md) through an explicit binding.
5. [Explore runnable examples](examples.md) and configure [Vim/Neovim](vim.md) or [VS Code](vscode.md).

## A first program

```text
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

Global `test` blocks run at compile time. Here, the block computes a constant and checks it before the program can build. Runtime execution prints the greeting and `42`.

```sh
argonc new hello
cd hello
argonc run
```

## What the current language has

**[Explore all implemented features, with runnable examples →](features.md)**

| Area | Implemented in Argon 0.1.0 |
| --- | --- |
| Types and variables | `i32`, `i64`, `f64`, `bool`, `string`; mutable `let`, `const`, inferred local types, numeric casts |
| Functions and modules | Typed `fn` declarations, recursion, explicit returns, pipelines, local imports, private-by-default declarations and `pub` exports |
| Control flow | `if` / `else`, `while`, C-style `for`, `break`, `continue`, and non-fallthrough `switch` |
| Pattern matching | Tagged enums with payloads and exhaustive `match` expressions |
| User-defined data | Value structs and reference-counted classes |
| Pointers and memory | Shared `Ptr<T>`, `Weak<T>`, checked dereferencing, weak upgrade, reset, and owner counts |
| Collections | Arrays, linked lists, deques, stacks, queues, hash maps, and hash sets; shared storage and explicit cloning |
| Metaprogramming | Global `test` blocks, scalar function evaluation, generated constants, and compile-time assertions |
| Builtins | Console input/output, string conversion, length, assertions, numeric casts, and memory helpers |
| Checked behavior | Integer overflow, narrowing, division, finite floating-point results, collection bounds, and empty pointer access |
| C++ bindings | Quoted `cpp "header";` includes, explicit scalar bindings, `unsafe` calls, automatic `cpp/` source discovery, and library linking |
| Build system | Project scaffolding, checking, native compilation, running, debug builds, and C++ export through `argonc` |
| Editors | Vim/Neovim and VS Code syntax highlighting, check/run/build commands, and native debugger integration |

The [implemented-features guide](features.md) shows how to use each of these. The [current-status page](status.md) records their limits and what is still missing.

Argon has its own syntax and a restricted set of capabilities. “Subset of C++” describes its intended scope and implementation relationship; an Argon source file is not valid C++ source. Including a C++ header does not automatically import its functions or object types; explicit bindings define the supported interface.

## From a small project to a larger C++ application

The long-term goal is to start with Argon’s simple workflow and migrate to independently maintained C++ when a project needs greater control. That future export should produce idiomatic C++, organized source files, required runtime support, and a standalone build configuration.

Today, `argonc emit-cpp` exports standalone generated C++. It does **not** yet produce an idiomatic, maintainable C++ project. Native dependencies still need their headers, sources, and link settings. See the [export instructions](build.md#export-and-the-migration-goal).

## Project purpose

The intended use case is a small-to-medium new application that needs native C++ functionality without maintaining a separate application build system. Argon emphasizes quick project creation, checked application code, reference counting, and a narrow explicit library boundary. That is the project's intended distinction from broader C++ successor efforts such as Carbon, rather than a feature or performance comparison.

The project is licensed under [Apache-2.0](https://github.com/RohanGoparaju028/Argon/blob/main/LICENSE).
