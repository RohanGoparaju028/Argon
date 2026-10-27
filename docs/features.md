---
title: Implemented features
---

# What Argon supports today

This is the feature inventory for **Argon 0.1.0**. Each feature below is implemented in the current compiler. Examples with `fn main()` are complete programs: save one as `main.ar` and run `argonc run main.ar`.

For shorter, independent examples—including each collection separately—see [Use each language feature in the README](https://github.com/RohanGoparaju028/Argon#use-each-language-feature).

The [keyword reference](keywords.md) defines all 33 reserved words, including the reserved-but-unimplemented `in`, and provides complete downloadable programs.

## At a glance

| Area | Available now |
| --- | --- |
| Source syntax | `.ar` files, braces, semicolons, `//` and `/* ... */` comments |
| Values | `i32`, `i64`, `f64`, `bool`, `string`, and `void` function results |
| Variables | Mutable `let`, immutable bindings with `const`, inferred local types, explicit type annotations |
| Functions | `fn`, typed parameters/results, recursion, explicit `return`, unary pipelines with `\|>` |
| Visibility | Private by default; explicit `pub` exports and optional `priv` |
| Modules | Local file imports with `import`, qualified names, ambiguity/cycle checks |
| Control flow | `if` / `else`, `while`, C-style `for`, `break`, `continue` |
| Branching | Tagged enums, payload binding and exhaustive `match`; literal `switch` cases |
| User types | Value `struct` types and reference-counted `class` objects |
| Memory | Shared owning `Ptr<T>`, observing `Weak<T>`, dereferencing, weak upgrade, reset, owner counts |
| Collections | `Array`, `LinkedList`, `Deque`, `Stack`, `Queue`, `HashMap`, `HashSet` |
| Compile-time work | Global `test` blocks, scalar function evaluation, generated constants, assertions |
| Builtins | Console I/O, scalar-to-string conversion, length, assertions, casts, memory helpers |
| C++ interop | Explicit scalar function bindings, `unsafe` calls, local native sources and library linking |
| Project tooling | `argonc new`, `init`, `check`, `build`, `run`, `emit-cpp`, and `doctor` |
| Editor tooling | Vim/Neovim and VS Code highlighting, checks, runners, and native debugger integration |

## Variables, types, and operators

`let` creates a mutable binding. `const` prevents assignment and mutation through that binding. Local types can be inferred; parameters need explicit types. Bindings must be initialized.

```text
fn main() -> i32 {
    let count = 2;
    count += 3;
    count++;
    const limit: i64 = 100i64;
    let fraction: f64 = 1.5;
    let ready: bool = count < limit && fraction > 0.0;
    let message: string = "count: " + str(count);
    println(message);
    assert(ready);
    return 0;
}
```

This prints `count: 6`.

| Type | Meaning |
| --- | --- |
| `i32` | Signed 32-bit integer |
| `i64` | Signed 64-bit integer |
| `f64` | Finite double-precision floating-point number |
| `bool` | `true` or `false` |
| `string` | Owned byte string; length counts bytes |
| `void` | A function returns no value |

Available operators include arithmetic `+ - * / %`, comparison `== != < > <= >=`, Boolean `! && ||`, assignment `= += -= *= /= %=`, and postfix `++` / `--`. `%` requires integers; string `+` concatenates. Boolean operators short-circuit. Conditions must be Boolean. Arguments and operands evaluate from left to right.

Numeric casts use `i32(value)`, `i64(value)`, and `f64(value)`. Integer arithmetic checks overflow; narrowing and integer casts check range and reject fractional values. Division checks zero, and floating-point results must remain finite. Integer-to-float conversion can lose precision.

See [types and operations](language.md#types-and-operations) for literal suffixes, string escapes, and conversion rules.

## Functions, visibility, and pipelines

Functions support typed parameters, explicit return values, recursion, and calls to other functions. A function with no result annotation returns `void`. A non-void function needs a return on every normal path.

```text
fn double(value: i32) -> i32 {
    return value * 2;
}

pub fn answer() -> i32 {
    return 21 |> double;
}

fn main() -> i32 {
    println(answer());
    return 0;
}
```

This prints `42`. `21 |> double` means `double(21)`; pipelines can be chained.

**Functions are private to their module by default.** `pub fn` exports a function for importing modules. `priv fn` explicitly states the default. `main` is recognized as the executable entry without `pub`; its required signature is `fn main() -> i32`, and its result is the exit status. Other declarations also use `pub` for module exports.

## Modules and imports

Projects can split code across local files. For example, put this in `math.ar`:

```text
pub fn add(left: i32, right: i32) -> i32 {
    return left + right;
}
```

Then put this in `main.ar` beside it:

```text
import math;

fn main() -> i32 {
    println(math.add(20, 22));
    return 0;
}
```

Run `argonc run main.ar` to print `42`. `import support.math;` resolves `support/math.ar`, with `.fe` as a fallback. Imports appear before declarations. Exported names can be accessed through short or qualified names; ambiguous short names and import cycles are rejected. Imported modules cannot define `main`.

## Loops, conditions, and switch

`if` / `else`, `while`, and C-style `for` loops use braces. `break` exits the nearest loop or switch; `continue` advances the nearest enclosing loop.

```text
fn main() -> i32 {
    let total = 0;
    for (let i = 0; i < 4; i++) {
        if (i == 1) { continue; }
        total += i;
    }
    while (total < 6) {
        total++;
    }
    switch (total) {
        case 6: println("six");
        default: println("other");
    }
    return 0;
}
```

This prints `six`. `switch` accepts integer, Boolean, and string values with literal cases. Cases do not fall through; a trailing `break` is optional. Each case has its own local scope.

## Tagged enums and pattern matching

Enums can have variants with typed payloads. `match` is an expression and returns a value. Enum matches must cover every variant or use `_`; Boolean matches must cover both values or use `_`. Other scalar matches require `_`.

```text
enum Result {
    Ok(value: i32),
    Err(message: string),
}

fn describe(result: Result) -> string {
    return match result {
        Result::Ok(value) => "value: " + str(value),
        Result::Err(message) => "error: " + message,
    };
}

fn main() -> i32 {
    println(describe(Result::Ok(42)));
    return 0;
}
```

This prints `value: 42`. Payload names are immutable within their arm. Duplicate patterns and arms after `_` are rejected. Current patterns are one level deep; nested patterns and guards are not implemented.

## Value structs and shared classes

A `struct` copies its value. A `class` allocates a reference-counted object; copying its handle shares that object. Constructors take every field in declaration order.

```text
struct Point { x: i32; y: i32; }
class Counter { value: i32; }

fn main() -> i32 {
    let point = Point(3, 4);
    let copy = point;
    copy.x = 9;
    assert(point.x == 3);

    let counter = Counter(1);
    let alias = counter;
    alias.value += 1;
    assert(counter.value == 2);
    println(counter.value);
    return 0;
}
```

This prints `2`. A struct with owning fields copies those handles, retaining the same referenced allocations. Recursive types can use class, collection, pointer, or weak-pointer links; direct recursive value layouts are rejected. Classes currently provide fields and shared ownership, without inheritance or user-defined instance methods.

## Reference counting, owning pointers, and weak pointers

`Ptr<T>` is a shared owning handle. `Weak<T>` observes an allocation without keeping its object alive. `*pointer` dereferences an owner; aggregate fields also support `pointer->field` and `pointer.field`. An object is destroyed when its last owner leaves scope, is reassigned, or is reset.

```text
fn main() -> i32 {
    let owner = Ptr<i32>(40);
    let alias = owner;
    *alias += 2;
    println(*owner);

    let observer = weak(owner);
    alias.reset();
    owner.reset();
    assert(is_expired(observer));
    let recovered = upgrade(observer);
    assert(is_empty(recovered));
    return 0;
}
```

This prints `42`, then verifies that the weak observer expires after both owners release the object. Weak observers can retain bookkeeping memory until they are dropped. They cannot be dereferenced directly; upgrade and check for an empty handle first.

The runtime checks empty pointer access. It exposes no raw addresses or manual `free`. **Strong reference cycles are not collected**; use weak back-references. `strong_count(handle)` and `.use_count()` report owners. See [memory and collections](memory.md) for pointer constructors, classes, nested pointers, and lifecycle details.

## Seven collection types

All collections start with empty, reference-counted storage. Copying a collection shares its storage; `.clone()` creates separate storage and copies its elements. Nested owning elements still share their allocations.

| Collection | Implemented operations |
| --- | --- |
| `Array<T>` | `push`, `pop`, `get`, `set`, `[index]`, `front`, `back` |
| `LinkedList<T>` | Array operations plus `push_front`, `pop_front`; indexing is linear-time |
| `Deque<T>` | Array operations plus `push_front`, `pop_front` |
| `Stack<T>` | `push`, `pop`, `top` |
| `Queue<T>` | `enqueue`, `dequeue`, `front` |
| `HashMap<K,V>` | `set`, `get`, `[key]`, `contains`, `remove`, `keys` |
| `HashSet<T>` | `insert`, `contains`, `remove`, `items` |

Every collection provides `length()`, `is_empty()`, `clear()`, `clone()`, `use_count()`, and `reset()`. Hash keys/set elements must be integers, Booleans, or strings. `keys()` and `items()` return sorted arrays.

```text
fn main() -> i32 {
    let values = Array<i32>();
    values.push(42);
    let alias = values;
    alias[0] += 1;
    assert(values[0] == 43);
    let copy = values.clone();
    copy[0] = 7;
    assert(values[0] == 43);

    let scores = HashMap<string, i32>();
    scores.set("Ada", values[0]);
    println(scores.get("Ada"));
    return 0;
}
```

This prints `43`. Index bounds, empty pops, and missing map keys are checked. `is_empty(handle)` tests whether an owning allocation exists; `collection.is_empty()` tests whether a live collection has zero elements.

## Compile-time metaprogramming with test

A global `test` block executes during checking or compilation. It supports scalar computations, local mutation, loops, conditionals, scalar matching, and ordinary Argon function calls. Direct `const` declarations become private module constants and need explicit types.

```text
fn square(value: i32) -> i32 {
    return value * value;
}

test {
    const computed: i32 = square(7);
    assert(computed == 49);
}

fn main() -> i32 {
    println(computed);
    return 0;
}
```

This prints `49`; a failed compile-time assertion stops the build. `test` is a metaprogramming block, not a runtime test declaration. There are no additional `meta` or `pure` keywords.

The evaluator limits execution to 100,000 steps and 64 nested function calls. It cannot perform I/O, invoke C++, or construct heap/aggregate objects. This provides bounded deterministic scalar evaluation for the same inputs and target behavior. General syntax-generating macros are not implemented.

## Builtin functions

| Functions | Purpose |
| --- | --- |
| `print(value)`, `println(value)`, `println()` | Write scalar values to standard output |
| `read_line()` | Read a line from standard input |
| `str(value)`, `string(value)` | Convert a scalar to a string |
| `length(value)` | Count string bytes or collection elements |
| `assert(condition)` | Check a Boolean condition at runtime or inside a compile-time block |
| `i32(value)`, `i64(value)`, `f64(value)` | Numeric conversion |
| `weak(owner)`, `upgrade(observer)` | Create a weak observer or recover ownership |
| `is_empty(owner)`, `is_expired(observer)` | Check allocation presence or weak expiration |
| `strong_count(handle)` | Inspect the owning reference count |

The compiler also accepts the convenience imports `std`, `std.io`, `std.core`, `std.collections`, `std.memory`, and `std.math`. They do not provide a full C++ standard library. Native mathematical APIs can be exposed through explicit C++ bindings.

## C++ interoperability available now

`cpp "header";` includes local or standard headers using the same quoted syntax. New projects contain `cpp/`; native source files placed there are compiled automatically. Generated code defaults to `argon-cxx/release/`, configurable with `--cxx-dir` or `[project] cxx_dir`.

`extern cpp fn` declares a binding to a named C++ function. Call it in an `unsafe` block. Supported boundary types are `i32`, `i64`, `f64`, and `bool`, with `void` also allowed as a result.

```text
cpp "demo.hpp";

extern cpp fn twice(value: i32) -> i32 = "demo::twice";

fn main() -> i32 {
    unsafe { println(twice(21)); }
    return 0;
}
```

This requires a matching C++ header/source and manifest configuration. `argonc new native-demo --cpp` generates that complete working project. The builder compiles and links it; native compilation verifies the exact function signature.

C++ wrappers may use complex libraries internally and expose the supported scalar interface. Direct C++ class, template, string, raw pointer, reference, callback, and automatic C++ API translation are not implemented. Read the [C++ guide](cpp.md) for the complete example and safety boundary.

## Compiler, build system, and editors

| Command | Available behavior |
| --- | --- |
| `argonc new DIRECTORY [--cpp]` | Scaffold an application, optionally with a C++ binding example |
| `argonc init [DIRECTORY] [--cpp]` | Scaffold inside an existing directory without overwriting generated files |
| `argonc check [FILE]` | Type-check and execute global compile-time blocks |
| `argonc build [FILE]` | Compile and link a native executable |
| `argonc run [FILE]` | Build, then run the executable |
| `argonc build --debug` | Produce a native debug build with source mappings |
| `argonc emit-cpp [FILE] -o program.cpp` | Export standalone generated C++ |
| `argonc doctor` | Locate the native C++ compiler |

Projects use `Argon.toml` and discover local source dependencies from imports. Native headers, sources, include directories, library directories, and library names can be configured. Each build currently recompiles; there is no package manager or incremental cache.

[Vim/Neovim](vim.md) and [VS Code](vscode.md) integrations provide highlighting and explicit checking, running, building, and debugger launch commands. Debugging uses native LLDB/CodeLLDB representations. See [current status](status.md) for editor/platform verification limits.

For precise rules, use the [language reference](language.md), [memory APIs](memory.md), and [build reference](build.md). For unsupported features and current verification limits, see [current status](status.md).
