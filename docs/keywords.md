---
title: Keyword reference
---

# Argon keyword reference

This page defines **every reserved word in Argon 0.1.0**, checked against the current parser and compiler on **27 October 2026**. There are **33 reserved words: 32 with implemented meanings and one reserved but unimplemented word, `in`**. Keywords are case-sensitive and cannot be used as your own variable, function, field, or declaration names.

Start with the index to look up a word. Each section then explains its syntax, allowed context, behavior, and relevant restrictions. The examples are complete programs with their expected output; they do not require you to invent missing functions or wrap fragments in `main`.

## Run the examples

Build/install `argonc` using [Getting started](getting-started.md). Save a complete example under its stated filename, or download it, then run the command shown. Except for the explicitly labeled two-file module example, each program needs just one `.ar` file. The native example uses the system C++ standard library and needs no downloaded library.

`argonc check FILE.ar` checks Argon types and executes global `test` blocks. `argonc run FILE.ar` also builds and runs the native program. All successful examples below exit with status `0`.

## Complete keyword index

| Word | Status | Meaning |
| --- | --- | --- |
| [`bool`](#bool) | Implemented | Boolean type. |
| [`break`](#break) | Implemented | Exit the nearest loop or switch. |
| [`case`](#case) | Implemented | Introduce a literal branch of a switch. |
| [`class`](#class) | Implemented | Declare a reference-counted object type. |
| [`const`](#const) | Implemented | Create an initialized immutable binding or module constant. |
| [`continue`](#continue) | Implemented | Advance the nearest enclosing loop. |
| [`cpp`](#cpp) | Implemented | Include a native header, or select C++ in a foreign declaration. |
| [`default`](#default) | Implemented | Introduce the fallback branch of a switch. |
| [`else`](#else) | Implemented | Provide an alternative to an if branch. |
| [`enum`](#enum) | Implemented | Declare a tagged union with named variants. |
| [`extern`](#extern) | Implemented | Declare a foreign function using `extern cpp fn`. |
| [`f64`](#f64) | Implemented | Finite double-precision floating-point type and numeric cast. |
| [`false`](#false) | Implemented | The false Boolean literal. |
| [`fn`](#fn) | Implemented | Declare a named function. |
| [`for`](#for) | Implemented | Run an initializer/condition/update loop. |
| [`i32`](#i32) | Implemented | Signed 32-bit integer type and checked numeric cast. |
| [`i64`](#i64) | Implemented | Signed 64-bit integer type and checked numeric cast. |
| [`if`](#if) | Implemented | Execute a block when a Boolean condition is true. |
| [`import`](#import) | Implemented | Load an Argon module. |
| [`in`](#in) | Reserved; not implemented | Reserved only; no implemented syntax. |
| [`let`](#let) | Implemented | Create an initialized mutable local binding. |
| [`match`](#match) | Implemented | Select a value using exhaustive patterns. |
| [`priv`](#priv) | Implemented | Explicitly keep a declaration private to its module. |
| [`pub`](#pub) | Implemented | Export a module declaration. |
| [`return`](#return) | Implemented | Exit the current function, optionally returning a value. |
| [`string`](#string) | Implemented | Owned byte-string type and scalar-to-string conversion. |
| [`struct`](#struct) | Implemented | Declare an aggregate with value-copy semantics. |
| [`switch`](#switch) | Implemented | Select a statement block using literal cases. |
| [`test`](#test) | Implemented | Execute a global block during checking/compilation. |
| [`true`](#true) | Implemented | The true Boolean literal. |
| [`unsafe`](#unsafe) | Implemented | Permit foreign calls in a lexical block. |
| [`void`](#void) | Implemented | Function result type for no value. |
| [`while`](#while) | Implemented | Repeat a block while a Boolean condition is true. |

## Functions and results

### fn

**Syntax:** `fn name(parameter: Type, ...) -> ResultType { ... }`

Declares a named function at module scope. Every parameter needs a type; the return type defaults to `void` if `-> ResultType` is omitted. Functions can call each other and recurse. Arguments are passed by value: scalar/struct values copy, and owning handles retain their shared allocation.

A function returning a value must explicitly return on every normal path. The last expression in a block is not an implicit return. Nested function declarations, closures, first-class function values, and user-defined generic functions are not implemented. An executable needs exactly `fn main() -> i32`, with no parameters.

### return

**Syntax:** `return expression;` or `return;`

Exits the current function. The expression must be compatible with the declared result type. Use bare `return;` for a `void` function. It can occur inside a function's nested blocks, including loops and switches, and still exits the function.

`return` cannot exit a global `test` block or appear as a module-level statement. A `return` inside a function called by a `test` block is allowed and returns to that caller. The result returned by `main` becomes the process exit status.

### void

**Syntax:** `fn action() -> void { ... }`

Indicates that a function returns no value. Omitting the return annotation has the same meaning. A `void` function may reach its closing brace or use `return;`. It cannot return a value, and `void` cannot be stored in a variable, parameter, collection element, or field.

#### Example: functions and explicit returns

**File: `functions.ar`** · [Download](downloads/keywords/functions.ar)

```text
fn add(left: i32, right: i32) -> i32 {
    return left + right;
}

fn announce(value: i32) -> void {
    println(value);
    return;
}

fn main() -> i32 {
    announce(add(20, 22));
    return 0;
}
```

```sh
argonc run functions.ar
```

Expected output:

```text
42
```

## Bindings and constants

### let

**Syntax:** `let name = expression;` or `let name: Type = expression;`

Creates a mutable local binding in a function or `test` block, including nested blocks and a `for` initializer. A value is required at declaration time. The type is inferred from that value when no annotation is supplied; it does not change after inference. Assignment and compound updates must respect that type.

Module-level mutable `let` declarations and uninitialized declarations such as `let count: i32;` are not supported.

### const

**Syntax:** `const name = expression;` or `const name: Type = expression;`

Creates an immutable binding. Local constants can be initialized from runtime expressions. Module constants are evaluated at compile time; their initializer may use scalar constant expressions and other constants. Calls to Argon functions for compile-time computation belong inside a global `test` block.

A direct `const` inside a global `test` block requires an explicit type and becomes a private module constant. Nested constants remain local to their block. A `pub const` outside the `test` can export a computed value.

`const` prevents assignment or mutation through that binding. It does not make an allocation deeply immutable: another mutable alias can still modify shared contents.

#### Example: mutable local value and constants

**File: `bindings.ar`** · [Download](downloads/keywords/bindings.ar)

```text
const base: i32 = 40;

fn main() -> i32 {
    let count = base;
    count += 2;
    const label: string = "answer";
    println(label + ": " + str(count));
    return 0;
}
```

```sh
argonc run bindings.ar
```

Expected output:

```text
answer: 42
```

## Visibility and Argon modules

### pub

**Syntax:** `pub fn ...`, `pub const ...`, `pub struct ...`, `pub class ...`, or `pub enum ...`

Exports a module-level declaration so importing modules can use it. It also works with foreign declarations: `pub extern cpp fn ...`. Declarations are private by default. `main` does not need `pub` to serve as the executable entry point.

`pub` is not an access modifier for individual fields, local variables, or `test` blocks. Field-level visibility and class methods are not implemented.

### priv

**Syntax:** `priv fn ...`, `priv const ...`, `priv struct ...`, `priv class ...`, or `priv enum ...`

Explicitly makes a declaration private to its module. This is the default when neither visibility keyword is present. Private functions may be called by other functions in the same module but are not exposed to importers. `priv extern cpp fn ...` is also allowed.

Do not combine `pub` and `priv` on one declaration. Neither keyword controls visibility of struct/class fields.

### import

**Syntax:** `import module;` or `import path.to.module;`

Loads an Argon module. `import support.math;` looks for `support/math.ar`, with `.fe` as a fallback, beside the importing file and then beside the entry file. Imports must appear in the initial import/header-declaration section before other declarations. They may be interspersed with `cpp "header";` declarations in that initial section.

Only `pub` declarations are available from another module. Exports can be referenced by a short name, the final module component, or the full path: for example `add`, `math.add`, or `support.math.add`. Ambiguous short names and import cycles are errors. Imported modules cannot declare `main`.

`import` loads Argon source; it does not download packages or import a C++ header. The built-in convenience module names are described in [Modules and builtins](language.md#modules-and-builtins).

#### Example: public API and a private helper

Create a directory named `modules` containing these **two files**. They must be beside each other.

**File: `modules/math.ar`** · [Download](downloads/keywords/modules/math.ar)

```text
priv fn offset() -> i32 {
    return 2;
}

pub const base: i32 = 40;

pub fn answer() -> i32 {
    return base + offset();
}
```

**File: `modules/main.ar`** · [Download](downloads/keywords/modules/main.ar)

```text
import math;

fn main() -> i32 {
    println(math.answer());
    println(math.base);
    return 0;
}
```

Run from the directory containing `modules/`:

```sh
argonc run modules/main.ar
```

Expected output:

```text
42
40
```

`math.answer()` may call its private `offset()`. Calling `math.offset()` from `main.ar` is rejected because it is not exported.

## Scalar types and Boolean literals

The following type words are used in annotations, parameter/result types, and fields. Numeric type words also perform explicit casts when called. `string(value)` performs scalar-to-string conversion. There is no equivalent Boolean conversion function.

### i32

**Syntax:** `let value: i32 = 42;`, `42i32`, or `i32(number)`

A signed 32-bit integer, from −2,147,483,648 through 2,147,483,647. Unsuffixed integer literals use `i32` when they fit. Integer arithmetic checks overflow and zero divisors. Explicit conversion checks range; a floating-point input must be a whole number. `i32("42")` does not parse a string and is rejected.

### i64

**Syntax:** `let value: i64 = 42;`, `42i64`, or `i64(number)`

A signed 64-bit integer, from −9,223,372,036,854,775,808 through 9,223,372,036,854,775,807. An unsuffixed integer literal too large for `i32` uses `i64` if it fits. `i32` widens implicitly to `i64`. Explicit floating-point conversion checks range and rejects a fractional value.

### f64

**Syntax:** `let value: f64 = 2.5;`, `2.5f64`, or `f64(number)`

A finite double-precision floating-point value. Decimal/exponent literals infer `f64`. Integer values can convert implicitly to `f64`, potentially losing precision. Arithmetic and conversions reject non-finite results; remainder `%` is for integers only. Identical floating-point results across different targets are not guaranteed.

### bool

**Syntax:** `let enabled: bool = true;`

The Boolean type. Conditions for `if`, `while`, and `for` must be `bool`. Comparisons produce Boolean values; `!`, `&&`, and `||` operate on them, with short-circuit evaluation for `&&` and `||`.

Integers and strings do not implicitly become Booleans. `bool(value)` is not an implemented conversion; compare a numeric value explicitly, for example `value != 0`.

### true

The true Boolean literal. Use it as a value, condition, or Boolean `match`/`switch` pattern. It has type `bool`, not an integer type.

### false

The false Boolean literal. It has type `bool`, and `!false` evaluates to `true`. A Boolean match without a wildcard must handle both `true` and `false`.

### string

**Syntax:** `let label: string = "Argon";` or `string(value)`

An owned byte string. Literals use double quotes, preserve raw UTF-8 source bytes, and support `\n`, `\r`, `\t`, `\0`, `\"`, and `\\` escapes. `length` counts bytes, and comparisons are bytewise. `+` concatenates strings.

`string(value)` converts a scalar to a string, just like `str(value)`. Strings do not implicitly convert to numbers. Unicode escape syntax and Unicode identifiers are not implemented. String values currently cannot cross the C++ function-binding boundary.

#### Example: all scalar types and both Boolean literals

**File: `scalars.ar`** · [Download](downloads/keywords/scalars.ar)

```text
fn main() -> i32 {
    let count: i32 = 42;
    let large: i64 = 3000000000i64;
    let fraction: f64 = 2.5;
    let ready: bool = true;
    let failed: bool = false;
    let label: string = "Argon";
    let widened: i64 = i64(count);

    println(count);
    println(large);
    println(fraction);
    println(ready && !failed);
    println(label + " " + string(widened));
    assert(length(label) == 5);
    return 0;
}
```

```sh
argonc run scalars.ar
```

Expected output:

```text
42
3000000000
2.5
true
Argon 42
```

## User-defined data types

### struct

**Syntax:** `struct Name { field: Type; ... }`

Declares a module-level aggregate with value semantics. The constructor `Name(value, ...)` requires one value for every field, in declaration order. Fields are accessed with `value.field`. Copying the struct copies its fields; any owning handles among those fields retain their allocations.

There are no default field values, user-written constructors, instance methods, or field visibility modifiers. A struct cannot contain itself directly by value; recursion must cross a pointer, collection, class, or weak-pointer link.

### class

**Syntax:** `class Name { field: Type; ... }`

Declares a module-level reference-counted object type. It uses the same field and constructor syntax as `struct`, but construction allocates a shared object, and copying a class value retains the same object. Field mutation through one mutable alias is visible through the others.

The object is destroyed when its last owning handle is released. Strong reference cycles require weak back-references; they are not automatically collected. Inheritance, class methods, custom constructors/destructors, and field-level visibility are not implemented. See [Memory and collections](memory.md).

#### Example: copying a struct versus sharing a class

**File: `records.ar`** · [Download](downloads/keywords/records.ar)

```text
struct Point {
    x: i32;
    y: i32;
}

class Counter {
    value: i32;
}

fn main() -> i32 {
    let point = Point(3, 4);
    let copy = point;
    copy.x = 9;
    println(point.x);
    println(copy.x);

    let counter = Counter(1);
    let alias = counter;
    alias.value++;
    println(counter.value);
    return 0;
}
```

```sh
argonc run records.ar
```

Expected output:

```text
3
9
2
```

## Tagged values and pattern matching

### enum

**Syntax:** `enum Name { Variant, VariantWithData(field: Type), ... }`

Declares a tagged union at module scope. A variant can contain typed payload fields. Construct one using `Name::Variant` or `Name::Variant()` for no payload, and `Name::VariantWithData(value)` for a payload. Each payload constructor must receive the declared arguments.

Enums have value semantics, including retention of any owning payload fields. An enum must declare at least one variant. Explicit numeric discriminants and C++-style integer enum conversions are not implemented.

### match

**Syntax:** `match expression { pattern => expression, ... }`

Selects and returns a value. All arms must return compatible non-void values. Enum patterns can bind a variant's payload to immutable names scoped to the arm. Scalar patterns use literal values; `_` is the wildcard.

Enum matches must cover every variant or end in `_`; Boolean matches must cover both values or end in `_`. Other scalar matches always require `_`, even if the programmer believes all inputs have been listed. Duplicate patterns and arms after `_` are rejected.

Each arm is an expression, not a statement block. Nested destructuring, guards, ranges, and or-patterns are not implemented. Scalar matching is supported during compile-time evaluation; aggregate/enum matching is not.

#### Example: enum payloads and a scalar wildcard

**File: `matching.ar`** · [Download](downloads/keywords/matching.ar)

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
    let label = match 7 {
        0 => "zero",
        _ => "nonzero",
    };
    println(label);
    return 0;
}
```

```sh
argonc run matching.ar
```

Expected output:

```text
value: 42
nonzero
```

## Conditional execution

### if

**Syntax:** `if (condition) { statements }`

Executes its block when a Boolean condition is true. Parentheses around the condition and braces around the body are required. An `if` is a statement; it does not itself produce a value. Use `match` for value-producing branching.

### else

**Syntax:** `if (condition) { ... } else { ... }` or `else if (condition) { ... }`

Adds the alternative branch to an `if`. It cannot appear by itself. An `else` block requires braces; `else if` chains another conditional. Non-void functions using conditional returns still need a return on every normal path.

#### Example: an if/else chain

**File: `conditions.ar`** · [Download](downloads/keywords/conditions.ar)

```text
fn main() -> i32 {
    let score = 42;
    if (score >= 40) {
        println("passed");
    } else if (score >= 20) {
        println("retry");
    } else {
        println("failed");
    }
    return 0;
}
```

```sh
argonc run conditions.ar
```

Expected output:

```text
passed
```

## Loops and loop control

### while

**Syntax:** `while (condition) { statements }`

Checks its Boolean condition before each iteration, so the body may run zero times. Parentheses and braces are required. `break` and `continue` control the nearest enclosing loop. A runtime loop has no compile-time evaluator step limit; loops executed inside a `test` do.

### for

**Syntax:** `for (initializer; condition; update) { statements }`

Implements the C-style loop. The initializer runs once, the Boolean condition is checked before each iteration, and the update executes after each completed iteration, including an iteration ended by `continue`. The initializer can declare `let`/`const`; the loop binding is scoped to the loop. Each part may be omitted; an omitted condition means `true`.

This is the only implemented `for` form. Iteration using `for item in collection` is not supported.

### break

**Syntax:** `break;`

Exits the nearest enclosing loop or `switch`. It does not return from a function. Inside a `switch` nested in a loop, it exits the switch, not the loop. Labels and `break value` are not implemented. Outside a loop/switch it is an error.

### continue

**Syntax:** `continue;`

Advances the nearest enclosing loop. In a `for`, control proceeds to the update expression before checking the condition again; in a `while`, it proceeds to the condition. A `continue` in a switch inside a loop continues that loop. It is an error when no loop encloses it, even if it is inside a switch.

#### Example: for, continue, break, and while

**File: `loops.ar`** · [Download](downloads/keywords/loops.ar)

```text
fn main() -> i32 {
    let total = 0;
    for (let i = 0; i < 6; i++) {
        if (i == 1) { continue; }
        if (i == 4) { break; }
        total += i;
    }
    println(total);
    while (total < 7) {
        total++;
    }
    println(total);
    return 0;
}
```

```sh
argonc run loops.ar
```

Expected output:

```text
5
7
```

## Statement-oriented branching

### switch

**Syntax:** `switch (value) { case literal: statements ... default: statements }`

Selects a statement branch using an integer, Boolean, or string value. Each arm has its own local scope. Cases do not fall through, and a trailing `break` is optional. Unlike `match`, a switch does not yield a value or bind enum payloads.

The fallback is optional; without it, an unmatched value executes no branch. Floating-point and enum switches are not implemented. Use `match` for tagged enums.

### case

**Syntax:** `case literal: statements`

Introduces a branch inside a switch. The label must be a literal compatible with the switch value; identifiers and computed expressions are not accepted as labels. Negative numeric literals are supported. Duplicate case values are rejected.

### default

**Syntax:** `default: statements`

Introduces the fallback inside a switch, executed only if no case matches. At most one default is allowed. `default` is not used for match fallbacks; use `_ => expression` there.

#### Example: switch without implicit fallthrough

**File: `switches.ar`** · [Download](downloads/keywords/switches.ar)

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

```sh
argonc run switches.ar
```

Expected output:

```text
zero
one
other
```

## Compile-time execution

### test

**Syntax:** `test { statements }`

Declares a block at module/global scope that executes during `check`, `build`, `run`, and `emit-cpp`. Its executable statements are not emitted into the runtime program. It is a metaprogramming block, not a declaration for a runtime test runner.

Supported operations include scalar values, arithmetic, casts, local mutation, conditionals, loops, scalar matches/switches, ordinary Argon function calls, assertions, scalar-to-string conversion, and string length. A direct typed `const` becomes a private module constant; `let` and nested constants remain local. Use a separate `pub const` outside the block to export its computed value.

Evaluation is limited to 100,000 steps per compilation and 64 nested function calls. I/O, unsafe execution, C++ calls, heap allocations, and aggregate construction are unavailable. Dependencies execute before their importers, and blocks within a module execute in source order. A test constant cannot be read before its binding executes. Failed assertions stop compilation.

`test` cannot be nested inside a function. It has no function name or parameters; `pub test`, `meta`, and `pure` modifiers are not supported.

#### Example: compute, validate, and export a constant

**File: `metaprogramming.ar`** · [Download](downloads/keywords/metaprogramming.ar)

```text
fn square(value: i32) -> i32 {
    return value * value;
}

test {
    let input = 7;
    const computed: i32 = square(input);
    assert(computed == 49);
}

pub const answer: i32 = computed;

fn main() -> i32 {
    println(answer);
    return 0;
}
```

```sh
argonc run metaprogramming.ar
```

Expected output:

```text
49
```

You can also run `argonc check metaprogramming.ar`: the assertion executes during checking, without running `main` or requiring the native compiler. It prints a check-success diagnostic, not `49`. Changing the assertion to `assert(computed == 50);` makes checking fail.

## Native C++ integration

### cpp

**Header syntax:** `cpp "header";`

Includes a native header at module scope. Both local and standard headers use this one quoted form. Lookup searches the project’s `cpp/` directory, configured include directories, and then the native compiler’s standard include paths. For example, `cpp "demo.hpp";` can select `cpp/demo.hpp`, while `cpp "cmath";` can select a system header. Header declarations from imported Argon modules contribute to generated C++ too.

Header declarations can appear in the initial import/header section or among other module declarations. They cannot appear inside a function or `test`. Names must be relative include names: absolute paths and parent traversal are rejected. Header existence and native API signatures are checked during native compilation, not `argonc check`.

**Foreign-declaration syntax:** `extern cpp fn ...`

Here `cpp` selects the native binding language. Merely including a header does not automatically import its types/functions into Argon. Existing Argon builtins do not need includes. There is no `cpp <header>;` syntax and no raw inline `cpp { ... }` block.

### extern

**Syntax:** `extern cpp fn local_name(parameters) -> ResultType = "namespace::function";`

Declares a native function at module scope. Parameters and the result use Argon types, while the quoted target names the C++ function. The declaration ends with `;` and has no Argon body. It can be prefixed with `pub` or `priv`.

The current boundary accepts `i32`, `i64`, `f64`, and `bool` arguments, with those types or `void` results. The generated bridge checks the exact native function-pointer signature during native compilation. It does not silently select an unrelated overload.

Standalone `extern fn`, foreign variables, callbacks, variadic arguments, C++ classes, strings, pointers, references, and template bindings are not implemented. C functions may use this binding mechanism when their headers expose compatible C linkage to C++. See the [native integration guide](cpp.md) for complete copied-file examples.

### unsafe

**Syntax:** `unsafe { statements }`

Permits C++ function calls lexically inside the block. Such calls are rejected outside it. An unsafe block may appear inside a function and can contain ordinary Argon statements. It does not disable type checking, bounds checks, arithmetic checks, or reference counting.

There is no `unsafe fn` modifier. A normal Argon function can contain an unsafe block, so its callers do not automatically need an unsafe block too. Foreign code can still crash or corrupt memory, and wrapping it does not prove it safe. Unsafe blocks cannot execute during compile-time evaluation.

#### Example: a standard-library header and native function

**File: `native.ar`** · [Download](downloads/keywords/native.ar)

```text
cpp "cmath";

extern cpp fn square_root(value: f64) -> f64 = "std::sqrt";

fn main() -> i32 {
    unsafe {
        println(square_root(81.0));
    }
    return 0;
}
```

```sh
argonc run native.ar
```

Expected output:

```text
9
```

For copied libraries, `argonc new app --cpp` scaffolds a complete example. Native sources in `cpp/` are compiled automatically, and output defaults to `argon-cxx/release/`. Use `--cxx-dir` to select another output directory. See [projects and builds](build.md) for the full workflow.

## Reserved but not implemented

### in

`in` is reserved by the parser, so it cannot be used as an identifier. It currently has **no implemented operation**: it does not provide a membership test or a range-based loop. Syntax highlighting may color it because it is reserved; that is not evidence of language support.

**Intentionally invalid example — this must fail:**

```text
fn main() -> i32 {
    let values = Array<i32>();
    for (item in values) {
        println(item);
    }
    return 0;
}
```

Save it as `unsupported-in.ar` and run `argonc check unsupported-in.ar`. The current parser rejects the loop. Use an indexed C-style `for` loop instead; maps/sets provide `keys()`/`items()` arrays when you need traversal.

## Builtins, types, and symbols that are not keywords

These names/symbols are implemented, but they are not additional reserved words in the keyword index:

| Name or symbol | What it is |
| --- | --- |
| `main` | The conventional executable entry function name; not a keyword |
| `print`, `println`, `read_line` | Builtin console functions |
| `assert`, `str`, `length` | Builtin assertions, conversion, and length operations |
| `weak`, `upgrade`, `is_empty`, `is_expired`, `strong_count` | Builtin memory/ownership helpers |
| `Ptr<T>`, `Weak<T>` | Builtin owning/observing type constructors |
| `Array<T>`, `LinkedList<T>`, `Deque<T>`, `Stack<T>`, `Queue<T>`, `HashMap<K,V>`, `HashSet<T>` | Builtin collection type constructors |
| `_` | Wildcard pattern in a match arm |
| `=>` | Separates a match pattern from its result expression |
| `::` | Selects an enum variant; appears inside quoted native names too |
| `->` | Introduces a function result type or accesses a pointer's field |
| `*pointer` | Checked pointer dereference |
| `\|>` | Calls a one-argument function with the value on its left |

`string` has both a type and conversion role but is still a reserved word. Collection names and builtin names are compiler-recognized APIs; they do not imply support for arbitrary user-defined generics or user-defined methods.

## Common mistakes and their supported forms

| Attempt | Current rule |
| --- | --- |
| `fn main() { ... }` for an executable | Use `fn main() -> i32` and return an exit code |
| `let x: i32;` | Initialize the binding, such as `let x: i32 = 0;` |
| Top-level `let x = 1;` | Use a module constant or a local binding |
| `if (1) { ... }` | Conditions require a Boolean expression |
| Calling another module's unmarked function | Export it with `pub` |
| `test` inside `main` | Place compile-time blocks at module scope |
| `case` inside `match` | Use `pattern => expression` |
| `default` inside `match` | Use `_ => expression` |
| Assuming switch cases fall through | Each selected branch ends automatically |
| `for (item in values)` | Use the C-style `for` form |
| `cpp <cmath>;` | Use `cpp "cmath";` |
| Calling a foreign function normally | Put the call in an `unsafe { ... }` block |
| Including a C++ header and immediately using its objects | Headers do not automatically create Argon bindings; the scalar boundary still applies |

For expression precedence, numeric rules, imports, and value semantics, continue with the [language reference](language.md). For concrete data structures, use [memory and collections](memory.md). For what remains unimplemented, see [current status](status.md).
