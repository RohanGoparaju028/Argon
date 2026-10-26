---
title: Examples
---

# Argon examples

From the repository root, run `./build/argonc run examples/NAME.ar` (or use the editor's Argon runner).

| File | Demonstrates |
| --- | --- |
| [tour.ar](https://github.com/RohanGoparaju028/Argon/blob/main/examples/tour.ar) | Compile-time blocks, tagged enums, matches, structs, loops, arrays, switches |
| [pointers.ar](https://github.com/RohanGoparaju028/Argon/blob/main/examples/pointers.ar) | Owning pointers, shared mutation, weak upgrade, expiry, and checked access |
| [family.ar](https://github.com/RohanGoparaju028/Argon/blob/main/examples/family.ar) | Reference-counted classes with a weak parent link to avoid cycles |
| [linked_nodes.ar](https://github.com/RohanGoparaju028/Argon/blob/main/examples/linked_nodes.ar) | A recursive struct linked by typed owning pointers |
| [collections.ar](https://github.com/RohanGoparaju028/Argon/blob/main/examples/collections.ar) | Maps, sets, linked lists, stacks, queues, and deques |
| [shared_collections.ar](https://github.com/RohanGoparaju028/Argon/blob/main/examples/shared_collections.ar) | Shared storage, explicit cloning, and weak collection observers |
| [debugging.ar](https://github.com/RohanGoparaju028/Argon/blob/main/examples/debugging.ar) | A small program for source breakpoints and stepping |

`argonc new native-demo --cpp` creates a complete additional example with an Argon caller and C++ header/source compiled together by the build tool. See [C++ interoperability](cpp.md).

## Read and run the tour

Save the following as `tour.ar`, then run `argonc run tour.ar`. It prints `720`, `value: 42`, and `four` on separate lines.

```text
struct Point {
    x: i32;
    y: i32;
}

enum Result {
    Ok(value: i32),
    Err(message: string),
}

fn factorial(n: i64) -> i64 {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

test {
    let n = 6;
    const computed: i64 = factorial(n);
    assert(computed == 720);
}

fn describe(result: Result) -> string {
    return match result {
        Result::Ok(value) => "value: " + str(value),
        Result::Err(message) => "error: " + message,
    };
}

fn main() -> i32 {
    println(computed);
    let point = Point(3, 4);
    let copy = point;
    copy.x = 9;
    assert(point.x == 3);

    let values = Array<i32>();
    for (let i = 0; i < 3; i++) {
        values.push(i * 2);
    }
    values[1] += 40;
    println(describe(Result::Ok(values[1])));

    switch (point.y) {
        case 4: println("four"); break;
        default: println("other");
    }
    return 0;
}
```
