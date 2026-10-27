---
title: Language reference
---

# Argon language reference

This reference describes the new C++17 implementation, version 0.1.0. Its executable is `argonc`. For the definition, scope, restrictions, and runnable examples of every reserved word, use the [complete keyword reference](keywords.md).

## Functions and bindings

```text
fn add(left: i32, right: i32) -> i32 {
    return left + right;
}

fn main() -> i32 {
    let number = add(20, 22);
    const greeting: string = "Hello!";
    println(greeting);
    println(number);
    return 0;
}
```

Parameters require types. Results default to `void`; blocks do not implicitly return their last expression. Non-void functions must return on every possible normal path. The executable entry is exactly `fn main() -> i32` with no arguments. Its return value becomes the process exit status. Uncaught checked runtime errors print a diagnostic and exit with status 1.

`let` is mutable; `const` prevents assignment and mutation through that binding. Other mutable aliases can still change shared contents. Bindings require initialization. Local types can be inferred. Parameters are passed by value: owning handles retain their shared allocation, while structs and scalars copy values. All names are case-sensitive; identifiers use ASCII letters, digits, and underscores, starting with a letter or underscore. The `ag_` prefix and identifiers containing double underscores are reserved for generated code.

## Visibility

Functions, structs, classes, enums, and constants are private to their module by default. Prefix a declaration with `pub` to make it available to importing modules. `priv` explicitly states the default. `main` is recognized as the executable entry without `pub`; it is not automatically a public library function.

```text
fn helper() -> i32 { return 10; }
pub fn answer() -> i32 { return 42; }
fn main() -> i32 { return helper(); }
```

## Types and operations

| Type | Behavior |
| --- | --- |
| `i32` | Signed 32-bit integer |
| `i64` | Signed 64-bit integer |
| `f64` | Finite double-precision floating-point number |
| `bool` | `true` or `false` |
| `string` | Owned byte string; raw UTF-8 source bytes are preserved |
| `void` | No return value; cannot be stored |
| `Array<T>` and other collections | Reference-counted shared storage |
| `Ptr<T>` | Checked owning heap handle |
| `Weak<T>` | Non-owning observer; upgrade before access |
| A struct or enum name | Aggregate value; owning fields retain their allocations |
| A class name | Reference-counted heap object |

Integer literals infer `i32` if they fit, otherwise `i64`. Decimal/exponent literals infer `f64`. Literal suffixes `i32`, `i64`, and `f64` fix the type. Strings use double quotes and support `\n`, `\r`, `\t`, `\0`, `\"`, and `\\` escapes. Strings are compared bytewise; `length` counts bytes. Unicode escapes and Unicode identifiers are not implemented.

Implicit numeric conversions are `i32` to `i64`, and integer to `f64`. Explicit `i32(value)`, `i64(value)`, and `f64(value)` casts accept numbers. Narrowing casts check their range, and integer casts reject fractional floating-point values. Integer-to-float conversion can lose precision. No implicit string/number or number/bool conversion exists.

Arithmetic uses `+ - * / %` and unary `+ -`. Integer arithmetic checks overflow; division and remainder check zero divisors. Floating-point arithmetic and casts reject non-finite results. Remainder requires integers. String `+` concatenates strings. Comparisons use `== != < > <= >=` on compatible scalar values; boolean logic uses `! && ||`. Conditions require `bool`. Boolean operators short-circuit.

Mutable bindings support `=`, `+=`, `-=`, `*=`, `/=`, `%=`, and postfix `++` / `--`. Postfix updates return the old value. Operands and function arguments evaluate from left to right. Assignment evaluates destination indices once, reads the old value for compound assignments, evaluates the right-hand side, then revalidates and writes the current destination. No native reference can escape a collection or pointer operation; reads return values or owning handles.

`value |> function` calls `function(value)`; chains associate left to right. Functions are not currently first-class values.

## Global metaprogramming with test

```text
fn factorial(n: i64) -> i64 {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

test {
    let input = 6;
    const computed: i64 = factorial(input);
    assert(computed == 720);
}

fn main() -> i32 {
    println(computed);
    return 0;
}
```

`test` blocks are allowed only at module/global scope. They run during `check`, `build`, `run`, and `emit-cpp`. They are metaprograms, not runtime unit-test declarations. Their executable block bodies are absent from generated runtime code.

Direct `const` bindings in a `test` block require explicit type annotations and become private module constants. `let` bindings and nested constants remain local to that block. Generated constants must not collide with other module declarations. To export one, define a separate `pub const` referring to it outside the block. Ordinary top-level `const` initializers accept constant expressions and references to generated constants; calling an Argon function to generate a constant belongs inside `test`. Ordinary top-level mutable `let` bindings are not accepted.

The evaluator supports scalar values, arithmetic, casts, comparisons, local mutation, conditionals, loops, scalar `match`, `switch`, and ordinary Argon function calls. `assert` fails compilation when false. `str` and string `length` are available. I/O, C++ calls, unsafe blocks, arrays, and aggregate values are unavailable during compile-time execution. Attempts through helper functions are rejected when evaluated. There are no `meta` or `pure` modifiers.

Dependencies execute their test blocks before importing modules. Within a module, blocks run in source order. A test constant is unavailable until its declaring binding executes. Cyclic constants are rejected. Evaluation is limited to 100,000 steps per compilation and 64 nested function calls. `return` cannot exit a global test block.

## Structs, classes, pointers, and collections

```text
struct Point {
    x: i32;
    y: i32;
}

fn main() -> i32 {
    let first = Point(3, 4);
    let second = first;
    second.x = 9;
    assert(first.x == 3);

    let values = Array<i32>();
    values.push(42);
    println(values[0]);
    return 0;
}
```

Aggregate constructors require one argument for every field, in declaration order. `class` accepts the same field syntax but allocates a reference-counted heap object. Copying a class retains its allocation. Inheritance, visibility on fields, explicit constructors, and instance methods are not implemented. Recursive types are supported through classes, collections, `Ptr<T>`, and `Weak<T>`; direct recursive value layouts are rejected.

Arrays start empty and support `push(value)`, `pop()`, `get(index)`, `set(index, value)`, `length()`, and `is_empty()`. Indexing `array[index]` reads or writes an element. Negative/out-of-range indexing and popping an empty array raise checked runtime errors. Copying an array retains its shared storage; `clone()` creates independent storage and copies elements according to their own types. Owning elements and nested collections still share their allocations. Mutating methods require a mutable, stored receiver; `get` returns a value rather than an escaping reference. Heap storage is destroyed when its final owner is dropped. Strong cycles require weak links and are not automatically collected.

Additional collections are `LinkedList<T>`, `Deque<T>`, `Stack<T>`, `Queue<T>`, `HashMap<K,V>`, and `HashSet<T>`. `Ptr<T>` and `Weak<T>` provide checked owning and observing pointers. See [memory and collection APIs](memory.md) for their syntax, methods, cycle handling, and examples.

## Tagged enums and match

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
```

Construct payload variants with `Result::Ok(42)`. A variant without payload can be written `State::Ready` or `State::Ready()`. Match is an expression: every arm returns a value, and arm results must have compatible types. Payload bindings are immutable and scoped to their arm.

An enum match must cover every variant or finish with `_ => expression`. A boolean match must cover both `true` and `false` or use `_`. Integer, floating-point, and string literal matches require `_`. Duplicate patterns and arms after `_` are errors. Patterns are one level deep; nested destructuring, guards, or-patterns, and range patterns are not implemented.

## Control flow and switch

```text
fn main() -> i32 {
    for (let i = 0; i < 3; i++) {
        switch (i) {
            case 0: println("zero"); break;
            case 1: println("one");
            default: println("other");
        }
    }
    return 0;
}
```

`if (condition) { ... } else { ... }`, `while (condition) { ... }`, and C-style `for` loops require braces for their bodies. `break` exits the nearest loop or switch; `continue` advances the nearest enclosing loop, including when written inside a switch.

`switch` selects a literal `case` or `default` using integer, boolean, or string equality. Each arm has its own local scope. Cases do not fall through; a trailing `break` is optional. There can be only one default. Match provides value construction and payload patterns; switch provides statement-oriented branching.

## Modules and builtins

`import support.math;` loads `support/math.ar`, with `.fe` as a fallback. Search starts beside the importing file and then beside the entry file. Imports precede declarations. Explicitly `pub` declarations are exported; other declarations are private. Imported exports can use short names, the module's final component (`math.add`), or the full module path (`support.math.add`). Ambiguous short names and cycles are errors. Imported modules cannot declare `main`.

Memory builtins are `weak(owner)`, `upgrade(observer)`, `is_empty(owner)`, `is_expired(observer)`, and `strong_count(handle)`. `*pointer` dereferences an owning pointer; `pointer->field` and `pointer.field` access aggregate fields. Empty access reports a checked runtime error.

Other builtins are `print(value)`, `println(value)` / `println()`, `read_line()`, `str(value)`, `length(value)`, and `assert(condition)`, plus numeric casts. Printing and string conversion accept scalar values. `read_line` reads standard input. `import std;`, `std.io`, `std.core`, `std.collections`, `std.memory`, and `std.math` are accepted convenience imports; general mathematical functions use explicit C++ bindings. Qualified `std.io` and `std.core` builtin names are also recognized.

A global `cpp "header";` declaration includes a local or standard C++ header. Lookup searches the project’s `cpp/` folder, configured include directories, and native standard include paths. Header declarations do not implicitly create Argon function bindings. See [C++ interoperability](cpp.md) for `cpp "header";`, `extern cpp fn`, and `unsafe`. Async, closures, exceptions in Argon syntax, user-defined generics, borrowing, operators on aggregates, and custom destructors are outside this version.
