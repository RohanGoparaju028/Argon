---
title: Overview
---

# Small native projects. One command to build.

Argon is an experimental programming language for small projects that need C++ libraries. It combines its own typed syntax, checked operations, reference-counted heap storage, and an integrated compiler and project builder: **`argonc`**. The compiler is written in C++17 and emits C++17 for a native compiler to build.

**These docs describe version 0.1.0, the current prototype.** The language and tools are still evolving. Read the [current status and limitations](status.md) before choosing it for a project.

## Start here

1. [Install the compiler and create a project](getting-started.md).
2. [Learn the syntax](language.md), including private-by-default functions and `pub` exports.
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

## What Argon promises today

| Area | Current behavior |
| --- | --- |
| Building | `argonc` creates projects, checks sources, invokes the native compiler, and links local C++ sources or installed libraries. |
| Memory | Classes, owning pointers, and collections share reference-counted allocations. The final owner releases the object; weak links help avoid cycles. |
| Checked operations | Integer overflow, numeric narrowing, invalid collection access, and empty pointer access produce errors. |
| Metaprogramming | Global `test` blocks perform bounded scalar computations and generate constants. |
| C++ bindings | Explicit functions with numeric/Boolean arguments and numeric/Boolean/void results. Calls require `unsafe`. |
| Determinism | Compile-time evaluation is bounded and deterministic for the same inputs and target behavior. Runtime I/O and cross-platform floating-point results are outside that guarantee. |

Argon has its own syntax and a restricted set of capabilities. “Subset of C++” describes its intended scope and implementation relationship; an Argon source file is not valid C++ source. Arbitrary C++ headers and object APIs cannot be imported directly.

## Project purpose

The intended use case is a small new application that needs native C++ functionality without maintaining a separate application build system. Argon emphasizes quick project creation, checked application code, reference counting, and a narrow explicit library boundary. That is the project's intended distinction from broader C++ successor efforts such as Carbon, rather than a feature or performance comparison.

The project is licensed under [Apache-2.0](https://github.com/RohanGoparaju028/Argon/blob/main/LICENSE).
