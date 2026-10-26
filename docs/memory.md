---
title: Memory and collections
---

# Reference-counted memory and collections

Argon uses automatic reference counting for shared heap objects and collection storage. Strings and other native value buffers use ordinary C++ scope-based ownership. The runtime is written in C++17 and implements owning handles with `std::shared_ptr` and observers with `std::weak_ptr`.

| Source type | Copy behavior |
| --- | --- |
| Numbers, booleans, strings, structs, enums | Copy a value; any owning fields retain their allocations |
| `class Name` | Retain the same heap object |
| `Ptr<T>` | Retain the same heap allocation |
| `Weak<T>` | Copy an observer without retaining the object |
| Collections | Retain shared collection storage |

An object is destroyed when its last owner is overwritten, reset, or leaves scope. Weak observers can retain the bookkeeping allocation after object destruction until those observers are dropped too. No tracing garbage collector, `delete`, or manual `free` is exposed. `Ptr<T>` is a shared owning pointer, comparable to `std::shared_ptr<T>`; it is not a unique/move-only pointer. Raw pointers, stack addresses, pointer arithmetic, and `unique_ptr`-style ownership are not implemented.

## Owning and weak pointers

```text
let owner = Ptr<i32>(40);
let alias = owner;
assert(strong_count(owner) == 2);
*alias += 2;
println(*owner); // 42

let observer: Weak<i32> = weak(owner);
alias.reset();
let recovered = upgrade(observer);
if (!is_empty(recovered)) { println(*recovered); }
```

`Ptr<T>(value)` allocates a numeric, string, struct, enum, or pointer value. `Ptr<T>()` is empty. Dereferencing uses `*pointer`; pointer-to-aggregate fields accept `pointer->field` or `pointer.field`. Reads return values, never escaping native references. Empty access reports a checked runtime error. `pointer.get()` returns a value; `pointer.reset()` drops that binding's ownership. Pointer equality compares allocation identity, not contents.

`Weak<T>()` creates an empty observer; `Weak<T>(owner)` and `weak(owner)` observe a compatible owner. `upgrade(observer)` / `observer.upgrade()` returns a new owner if the allocation is alive, or an empty owning handle if it has expired. Test it with `is_empty` before accessing it. `is_expired(observer)` / `observer.is_expired()` checks expiration. Weak pointers cannot be dereferenced directly.

`strong_count(handle)` or `handle.use_count()` reports the current number of owners. Reading a stored handle through this builtin does not introduce another owner. Temporary owning values elsewhere in an expression may affect the count. Passing an owning handle to an ordinary function retains it for the call.

## Classes and cycle-safe object graphs

```text
class Person {
    name: string;
    parent: Weak<Person>;
    children: Array<Person>;
}

let parent = Person("Ada", Weak<Person>(), Array<Person>());
let child = Person("Grace", weak(parent), Array<Person>());
parent.children.push(child);
```

Class constructors still initialize every field in declaration order. Constructing a class allocates it; assignment and function arguments copy its owning handle. Classes have no inheritance, custom constructors, instance methods, or destructors in this version. Structs remain values; use `Ptr<Struct>` when sharing one on the heap.

Classes and collections are already owning types: `Ptr<Person>` is the same type as `Person`, and `Ptr<Array<i32>>` is the same type as `Array<i32>`. Constructing such a `Ptr` from an existing compatible handle retains it; constructing it without arguments produces an empty handle. `Weak<Person>` and `Weak<Array<i32>>` directly observe those allocations. `Ptr<Ptr<i32>>` can explicitly allocate a pointer value; its observer is `Weak<Ptr<i32>>`. For ordinary numeric/struct values, `Weak<T>` observes `Ptr<T>`.

Recursive types must cross a class, collection, owning-pointer, or weak-pointer link. A struct cannot contain itself directly by value.

**Strong cycles are not collected.** If a parent owns a child and that child owns its parent, both can remain alive forever. Use a weak link for the back-reference, as in [the family example](https://github.com/RohanGoparaju028/Argon/blob/main/examples/family.ar). Collections can also participate in cycles through their elements. Reference counting does not infer which link should be weak.

## Collections

All collection constructors take no arguments and create empty, reference-counted storage. Copying a handle shares mutations. `clone()` creates independent collection storage; elements are copied according to their own types, so pointers and nested collections still share their referenced allocations.

```text
let values = Array<i32>();
let alias = values;
alias.push(42);
assert(values[0] == 42);
let independent = values.clone();
independent[0] = 7;
assert(values[0] == 42);
```

| Collection | Main operations |
| --- | --- |
| `Array<T>` | `push`, `pop`, `get`, `set`, `[index]`, `front`, `back` |
| `LinkedList<T>` | Array operations plus `push_front`, `pop_front`; indexed access takes linear time |
| `Deque<T>` | Array operations plus `push_front`, `pop_front` |
| `Stack<T>` | `push`, `pop`, `top` |
| `Queue<T>` | `enqueue`, `dequeue`, `front` |
| `HashMap<K,V>` | `set(key,value)`, `get(key)`, `[key]`, `contains`, `remove`, `keys` |
| `HashSet<T>` | `insert`, `contains`, `remove`, `items` |

Every collection also provides `length()`, `is_empty()`, `clear()`, `clone()`, `use_count()`, and `reset()`. `length(collection)` is equivalent to its length method. `pop`, `pop_front`, and `dequeue` return the removed value. `insert` on a set returns whether a new item was added. `remove` returns whether the key existed. Keys/elements in hash maps/sets must be integers, booleans, or strings. `keys()` and `items()` return sorted arrays for deterministic traversal.

Indices are checked. Empty pops and missing map keys raise runtime errors. Indexing a missing map key does not insert an element; use `set` to insert. `Stack`, `Queue`, and `HashSet` do not expose indexing.

`collection.reset()` releases that handle, while `clear()` empties shared storage. After a weak upgrade, **`is_empty(handle)` checks whether an allocation exists**; `handle.is_empty()` checks whether a live collection contains zero elements. Accessing an empty owning collection reports an error. A newly constructed empty collection has an allocation and therefore `is_empty(collection)` is false.

`const` prevents assignment or mutation through that binding. A separate mutable alias can still change shared contents; this is not deep immutability or a thread-safety guarantee. The current language has no concurrent execution API, and the containers are not synchronized for concurrent mutation in foreign C++ code.

Heap objects and collections are unavailable inside compile-time `test` execution. Compile-time metaprograms remain bounded scalar computations. C++ bindings still accept only scalar signatures; raw native ownership is not silently imported into Argon.
