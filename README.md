# Argon

**A native language for small-to-medium projects that need C++ libraries without managing a separate C++ build system.**

[Documentation](https://rohangoparaju028.github.io/Argon/) · [Installation](docs/installation.md) · [Implemented features](docs/features.md) · [Keyword reference](docs/keywords.md) · [Language reference](docs/language.md) · [Current status](docs/status.md)

## Why Argon exists

For a small-to-medium application, configuring and maintaining a C++ build can take more effort than the application warrants. Argon aims to make starting that project straightforward: write application code in Argon, describe native dependencies in one manifest, and let `argonc` handle compilation and linking.

Argon has its own grammar and language rules. Its compiler is written in C++17 and translates Argon to C++17, which Clang or GCC compiles into a native executable. Interoperability is one-way: Argon calls explicitly bound C++ functions. C++ libraries do not need to understand Argon.

```text
Argon source + configured C++ sources and libraries
                       ↓
        argonc: check → generate C++ → compile → link
                       ↓
                Native executable
```

The long-term goal is **start with Argon's simple workflow, then move to maintainable C++ when the project needs greater control**. An eventual migration export should produce idiomatic C++, meaningful names, organized source files, the required runtime support, and a standalone build configuration.

**Current stage: experimental 0.1.0.** Project creation, native building, reference counting, and explicit scalar C++ bindings work today. `argonc emit-cpp` exports standalone generated C++, but an idiomatic, organized C++ project export is **planned, not implemented**. Complex C++ libraries may need hand-written wrappers.

Compiling through C++ does not make every error behave like ordinary C++. Argon reports its own syntax/type errors and adds checked arithmetic, pointer access, and collection access. Native compilation/linking errors come from the C++ toolchain. Foreign C++ code retains its own risks and behavior.

## Installation

Argon currently installs from source using Git, CMake 3.16+, and a C++17 compiler. The [complete installation guide](docs/installation.md) covers prerequisites, building, installation, permanent PATH setup, updates, and troubleshooting for:

| Operating system | Installation instructions |
| --- | --- |
| macOS | [Apple Command Line Tools + CMake](docs/installation.md#macos) |
| Linux | [Ubuntu/Debian, Fedora, and Arch Linux](docs/installation.md#linux) |
| Windows | [MSYS2 UCRT64 + MinGW-w64 GCC](docs/installation.md#windows) |

On macOS/Linux, once the prerequisites are installed:

```sh
git clone https://github.com/RohanGoparaju028/Argon.git
cd Argon
cmake -S . -B build-install -DCMAKE_BUILD_TYPE=Release
cmake --build build-install --parallel 2
cmake --install build-install --prefix "$HOME/.local"
export PATH="$HOME/.local/bin:$PATH"
argonc --version
argonc doctor
```

Add the PATH export to your shell configuration to persist it. Windows users should follow the dedicated UCRT64 steps, including compiler selection and runtime DLL setup. Keep the C++ compiler installed: `argonc` uses it for application builds. Python and Node.js are not required by the compiler.

For development without installation, the compiler can also be built into `build/` and run as `./build/argonc`; the examples later in this README use that development path.

## Start a project anywhere

With `argonc` on PATH:

```sh
argonc new "$HOME/hello-argon"
cd "$HOME/hello-argon"
argonc run
```

Edit `src/main.ar` and run again. The project contains `Argon.toml`, `.gitignore`, `src/main.ar`, and a `cpp/` directory for native headers and implementations. `new` requires a new directory; `argonc init` initializes an existing directory without overwriting generated files.

| Command | Use |
| --- | --- |
| `argonc check` | Check types and execute global compile-time blocks |
| `argonc build` | Build `argon-cxx/release/NAME` |
| `argonc run` | Build and run the project |
| `argonc build --debug` | Build `argon-cxx/debug/NAME` with native debug information |
| `argonc build -o PATH` | Choose an executable output path whose parent exists |
| `argonc emit-cpp -o program.cpp` | Export standalone generated C++ |
| `argonc doctor` | Locate the native C++ compiler |

Commands discover the nearest `Argon.toml`. A single file also works: `argonc run example.ar`. `.ar` is the primary extension; `.fe` is accepted too. See [the build reference](docs/build.md) for manifest fields and native dependency configuration.

## Use each language feature

Each subsection below is independent. For a block containing only statements, place it inside this entry function:

```text
fn main() -> i32 {
    // Put one statement example here.
    return 0;
}
```

Examples that already contain `fn main()` are complete programs. Save one as `example.ar` and run `argonc run example.ar`. The imports and C++ examples specify their additional files.

### Variables and constants

`let` is mutable; `const` prevents mutation through that binding. All bindings need an initial value. Local types can be inferred.

```text
let count = 1;
count += 2;
const limit: i32 = 10;
assert(count < limit);
```

`const` does not make shared objects deeply immutable: another mutable alias can still change their contents.

### Basic types and numeric casts

The scalar types are `i32`, `i64`, `f64`, `bool`, and `string`. `void` is a function result type. Numbers support type suffixes and explicit casts.

```text
let small: i32 = 42;
let large: i64 = 100i64;
let fraction: f64 = 2.5;
let enabled: bool = true;
let label: string = "Argon";
let widened = i64(small);
let narrowed = i32(large);
assert(widened == 42i64 && narrowed == 100);
assert(enabled && fraction > 0.0);
println(label);
```

Narrowing checks range. Integer casts reject fractional values; integer-to-float conversion can lose precision. Strings contain bytes, preserve UTF-8 source bytes, and use double quotes. `//` and `/* ... */` comments are supported.

### Operators and checked arithmetic

Arithmetic uses `+ - * / %`, comparisons use `== != < > <= >=`, and Boolean logic uses `! && ||`. Assignment supports `= += -= *= /= %=` and postfix `++` / `--`.

```text
let value = 10;
value *= 3;
value--;
assert(value == 29);
assert(value % 2 == 1 && !(value < 0));
println("value: " + str(value));
```

Integer overflow, invalid narrowing, division by zero, and non-finite floating-point results are checked. Boolean operators short-circuit. Arguments and operands evaluate left to right.

### Functions and return values

Parameters require types. Use `-> TYPE` for a result and explicit `return`. Omitting the result annotation means `void`. Recursion is supported.

```text
fn add(left: i32, right: i32) -> i32 {
    return left + right;
}

fn greet() {
    println("Hello!");
}

fn main() -> i32 {
    greet();
    println(add(20, 22));
    return 0;
}
```

`main` must have no parameters and return `i32`; its result becomes the process exit status. Non-void functions must return on every normal path.

### Public/private declarations and modules

Declarations are private to their module by default. Add `pub` to export one; `priv` explicitly states the default. `main` does not need `pub`.

Create `math.ar`:

```text
priv fn offset() -> i32 { return 2; }

pub fn answer() -> i32 {
    return 40 + offset();
}
```

Create `main.ar` beside it:

```text
import math;

fn main() -> i32 {
    println(math.answer());
    return 0;
}
```

Run `argonc run main.ar`. Imports precede declarations. `import support.math;` resolves `support/math.ar`. Import cycles and ambiguous short names are rejected; imported modules cannot define `main`.

### Function pipelines

`value |> function` calls a one-argument function with that value. Pipelines associate left to right.

```text
fn double(value: i32) -> i32 { return value * 2; }
fn increment(value: i32) -> i32 { return value + 1; }

fn main() -> i32 {
    println(20 |> double |> increment);
    return 0;
}
```

This prints `41`. Functions are not first-class values in this version.

### If and else

Conditions require `bool`; branches use braces.

```text
let score = 42;
if (score >= 40) {
    println("passed");
} else {
    println("try again");
}
```

### While loops

```text
let remaining = 3;
while (remaining > 0) {
    println(remaining);
    remaining--;
}
```

### For loops, break, and continue

Loops use C-style `for` syntax. `continue` skips to the next iteration; `break` exits the nearest loop or switch.

```text
for (let i = 0; i < 5; i++) {
    if (i == 1) { continue; }
    if (i == 4) { break; }
    println(i);
}
```

This prints `0`, `2`, and `3`. Range-based `for ... in` loops are not implemented.

### Switch

`switch` selects literal integer, Boolean, or string cases. Cases do not fall through; a trailing `break` is optional.

```text
let command = "run";
switch (command) {
    case "run": println("running");
    case "check": println("checking");
    default: println("unknown");
}
```

### Enums and match

Tagged enums can carry typed payloads. `match` is an expression that returns a value and binds variant payloads.

```text
enum Result {
    Ok(value: i32),
    Err(message: string),
}

fn main() -> i32 {
    let result = Result::Ok(42);
    let text = match result {
        Result::Ok(value) => "value: " + str(value),
        Result::Err(message) => "error: " + message,
    };
    println(text);
    return 0;
}
```

Enum matches must cover every variant or use `_`. Boolean matches must cover both values or use `_`; other scalar matches require `_`. Payload bindings are immutable. Patterns are one level deep; guards and nested patterns are not supported.

### Scalar match and wildcard

```text
let code = 2;
let label = match code {
    1 => "one",
    2 => "two",
    _ => "other",
};
println(label);
```

### Structs: value semantics

Constructors initialize every field in declaration order. Copying a struct copies its value; owning fields retain their referenced allocations.

```text
struct Point { x: i32; y: i32; }

fn main() -> i32 {
    let original = Point(3, 4);
    let copy = original;
    copy.x = 9;
    assert(original.x == 3);
    return 0;
}
```

### Classes: shared objects

A class constructor allocates a reference-counted object. Copies share that object. Classes currently have fields, without inheritance or user-defined instance methods.

```text
class Counter { value: i32; }

fn main() -> i32 {
    let counter = Counter(1);
    let alias = counter;
    alias.value++;
    assert(counter.value == 2);
    return 0;
}
```

### Owning pointers and dereferencing

`Ptr<T>` is a shared owning pointer. `*pointer` accesses the stored value; `.get()` reads a value. Aggregate fields also accept `pointer->field` and `pointer.field`.

```text
let owner = Ptr<i32>(40);
let alias = owner;
assert(strong_count(owner) == 2);
*alias += 2;
assert(owner.get() == 42);
alias.reset();
assert(owner.use_count() == 1);
```

An object is destroyed when its final owner is reset, overwritten, or leaves scope. Empty access is checked. There is no manual `free`, raw address access, pointer arithmetic, or unique/move-only pointer type.

### Weak pointers and memory release

A weak pointer observes an allocation without owning it. Upgrade it to an owner and check for emptiness before access.

```text
let owner = Ptr<i32>(42);
let observer: Weak<i32> = weak(owner);
{
    let recovered = upgrade(observer);
    if (!is_empty(recovered)) { println(*recovered); }
}
owner.reset();
assert(is_expired(observer));
assert(is_empty(upgrade(observer)));
```

Strong cycles are not collected. Use weak back-references for parent/child graphs; see [the family example](examples/family.ar). Weak observers may retain bookkeeping memory after the object has been destroyed.

### Array

```text
let values = Array<i32>();
values.push(10);
values.push(20);
values[0] = 11;
assert(values.get(0) == 11);
assert(values.pop() == 20);
assert(values.length() == 1);
```

Arrays also have `set`, `front`, and `back`. Indexing and empty pops are checked.

### LinkedList

```text
let values = LinkedList<i32>();
values.push(20);
values.push_front(10);
assert(values.front() == 10);
assert(values.pop_front() == 10);
assert(values[0] == 20);
```

Linked lists support the array operations plus front insertion/removal. Indexed access takes linear time.

### Deque

```text
let values = Deque<i32>();
values.push(20);
values.push_front(10);
assert(values.front() == 10);
assert(values.back() == 20);
assert(values.pop_front() == 10);
assert(values.pop() == 20);
```

A deque supports insertion and removal at both ends, plus indexed access.

### Stack

```text
let pending = Stack<string>();
pending.push("first");
pending.push("last");
assert(pending.top() == "last");
assert(pending.pop() == "last");
```

A stack is last-in, first-out.

### Queue

```text
let pending = Queue<string>();
pending.enqueue("first");
pending.enqueue("last");
assert(pending.front() == "first");
assert(pending.dequeue() == "first");
```

A queue is first-in, first-out.

### HashMap

```text
let scores = HashMap<string, i32>();
scores.set("Ada", 42);
assert(scores.contains("Ada"));
scores["Ada"] += 1;
assert(scores.get("Ada") == 43);
let names = scores.keys();
assert(names[0] == "Ada");
assert(scores.remove("Ada"));
```

Keys must be integers, Booleans, or strings. `keys()` returns a sorted array. Reading a missing key is an error; use `set` to insert an entry.

### HashSet

```text
let tags = HashSet<string>();
assert(tags.insert("native"));
assert(!tags.insert("native"));
assert(tags.contains("native"));
let items = tags.items();
assert(items[0] == "native");
assert(tags.remove("native"));
```

Elements must be integers, Booleans, or strings. `items()` returns a sorted array.

### Shared collections and cloning

Copying a collection retains the same storage. `clone()` allocates separate storage and copies the elements; owning elements still share their referenced objects.

```text
let values = Array<i32>();
values.push(42);
let alias = values;
alias[0] = 43;
assert(values[0] == 43);
let independent = values.clone();
independent[0] = 7;
assert(values[0] == 43);
alias.clear();
assert(values.is_empty());
assert(!is_empty(values));
```

All seven collections provide `length`, `is_empty`, `clear`, `clone`, `use_count`, and `reset`. `collection.is_empty()` checks its size; `is_empty(collection)` checks whether its owning allocation exists. `clear()` changes shared contents; `reset()` releases one handle. See [the full memory API](docs/memory.md).

### Compile-time metaprogramming

Global `test` blocks run during `check`, `build`, `run`, and `emit-cpp`. A direct typed `const` in a block becomes a private module constant. Temporary `let` bindings remain local.

```text
fn square(value: i32) -> i32 { return value * value; }

test {
    const computed: i32 = square(7);
    assert(computed == 49);
}

fn main() -> i32 {
    println(computed);
    return 0;
}
```

Compile-time evaluation supports scalar computations, function calls, loops, local mutation, conditionals, scalar matching, and assertions. It cannot perform I/O, invoke C++, or construct heap/aggregate values. Execution is bounded to 100,000 steps and 64 nested function calls. `test` is not a runtime test declaration or a syntax-generating macro system.

### Output, strings, length, and assertions

```text
let message = "Hello";
print(message);
println(" from Argon!");
println();
assert(length(message) == 5);
println(str(42));
```

Printing and string conversion accept scalar values. `str(value)` and `string(value)` convert scalars to strings. `length` counts string bytes or collection elements. Assertions also work inside compile-time `test` blocks.

### Console input

```text
print("Your name: ");
let name = read_line();
println("Hello, " + name);
```

`read_line()` reads standard input. Programs using runtime input or foreign libraries are not promised identical output across runs.

## Choose the build-output directory

Generated C++ and default executables now go in the visible `argon-cxx/release/` directory, or `argon-cxx/debug/` with `--debug`. No new `.argon/` directory is created.

```sh
argonc new app --cxx-dir generated-cpp
argonc build --cxx-dir generated-cpp
argonc run --cxx-dir generated-cpp
argonc emit-cpp --cxx-dir generated-cpp
```

`new`/`init` save the setting as `[project] cxx_dir` in `Argon.toml`, relative to the project root. Build/run/export overrides are relative to the current directory; absolute paths also work. `argonc emit-cpp` without options writes to the configured release directory. Use `--stdout` to print it or `-o FILE.cpp` to name a file explicitly.

## Use C++ libraries

Every project has a `cpp/` folder. Copy headers and implementation files there. `argonc` recursively compiles `.cpp`, `.cc`, `.cxx`, `.c++`, and `.c` files automatically; C files compile separately as C11. Use one quoted syntax for local and standard headers:

```text
cpp "demo.hpp";          // cpp/demo.hpp
cpp "vendor/math.hpp";   // cpp/vendor/math.hpp
cpp "cmath";             // Native compiler's standard include paths
```

Lookup searches `cpp/` first, configured include directories next, and standard include paths last. Header-only libraries need no separate implementation file. Function bindings are still explicit: inclusion does not automatically expose every C++ type or function.


Create the complete native example:

```sh
argonc new native-demo --cpp
cd native-demo
argonc run
```

It prints `42`. The generated `src/main.ar` declares and calls the native function:

```text
cpp "demo.hpp";

extern cpp fn twice(value: i32) -> i32 = "demo::twice";

fn main() -> i32 {
    unsafe { println(twice(21)); }
    return 0;
}
```

The corresponding `Argon.toml` needs no native file list for this layout:

```toml
[project]
schema = 1
name = "native-demo"
entry = "src/main.ar"
cxx_dir = "argon-cxx"
```

Bindings currently accept numeric/Boolean arguments and numeric/Boolean/void results. Use `std::int32_t`, `std::int64_t`, `double`, and `bool` for exact native signatures. `argonc check` checks the Argon declaration; native compilation checks the C++ symbol and signature.

Calls require `unsafe`. C++ classes, strings, raw pointers, references, callbacks, and direct template binding are not supported across the boundary. A C++ wrapper can use a complex library internally and expose supported scalar functions. The builder can link local native sources and installed libraries, but does not install dependencies. See [the complete C++ guide](docs/cpp.md).

## Export generated C++ today

For a project without external native dependencies:

```sh
argonc emit-cpp -o program.cpp
c++ -std=c++17 program.cpp -o program
./program
```

The generated file includes Argon's runtime support and can be compiled without `argonc`. A project with C++ bindings still needs its headers, native sources, include paths, and libraries supplied to the C++ compiler.

**This is generated compiler output, not yet an idiomatic migration export.** Exporting a clean, maintainable, multi-file C++ project with a standalone build configuration is a future goal. Keeping Argon's checked behavior may require runtime helpers even in that future export.

## Use the editor integrations

### Vim and Neovim

Add the plugin to Vim's runtime path before enabling filetype plugins:

```vim
set runtimepath+=/absolute/path/to/Argon/editors/vim
filetype plugin on
syntax on
let g:argon_compiler = '/absolute/path/to/Argon/build/argonc'
```

Use `:ArgonCheck`, `:ArgonRun`, `:ArgonRunProject`, `:ArgonBuild`, or `:ArgonDebug`. Debugging needs LLDB; Neovim can optionally use nvim-dap and lldb-dap. [Installation and commands](editors/vim/README.md).

### VS Code

Build the extension package from the repository:

```sh
cd editors/vscode
npm run package
```

Use **Extensions: Install from VSIX**, select the generated `.vsix`, and set `argon.compilerPath` or put `argonc` on PATH. Use **Argon: Check File**, **Run File**, **Build Project**, or **Debug File**. Debugging needs CodeLLDB. [Installation and commands](editors/vscode/README.md).

Native source mappings and breakpoint resolution have been checked; live stepping was not verified on the development host. See [current status](docs/status.md) for editor/platform verification limits.

## Run the existing examples and tests

From the compiler repository:

```sh
ctest --test-dir build --output-on-failure
./build/argonc run examples/tour.ar
./build/argonc run examples/pointers.ar
./build/argonc run examples/family.ar
./build/argonc run examples/linked_nodes.ar
./build/argonc run examples/collections.ar
./build/argonc run examples/shared_collections.ar
./build/argonc run examples/native_files/src/main.ar
```

CTest includes additional editor/debugger tests when their tools are installed. See [the examples index](examples/README.md).

## Current boundaries

Argon's checks and reference counting reduce common errors, but this prototype has no formal memory-safety proof. Strong cycles need weak links. Foreign C++ can corrupt memory or crash. Compile-time scalar evaluation is deterministic for fixed inputs and target behavior; runtime I/O, cross-target floating-point results, and executable bytes are outside that guarantee.

Inheritance, instance methods, closures, async, borrowing, user-defined generics, automatic translation of C++ header APIs, package downloads, incremental compilation, general syntax-generating macros, and idiomatic C++ project migration export are not implemented. See [the current-status page](docs/status.md) for the complete scope.

Argon's intended distinction from broader C++ successor efforts such as Carbon is its focus on a low-setup application workflow and a future path to independently maintained C++. This is a statement of project goals, not a claim of feature or performance superiority.

Licensed under [Apache-2.0](LICENSE).
