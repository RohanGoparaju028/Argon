# Argon examples

From the repository root, run `./build/argonc run examples/NAME.ar` (or use the editor's Argon runner).

| File | Demonstrates |
| --- | --- |
| [native_files/](native_files/) | Complete project with local headers, C/C++ implementations, a header-only function, and a standard-library call |
| [tour.ar](tour.ar) | Compile-time blocks, tagged enums, matches, structs, loops, arrays, switches |
| [pointers.ar](pointers.ar) | Owning pointers, shared mutation, weak upgrade, expiry, and checked access |
| [family.ar](family.ar) | Reference-counted classes with a weak parent link to avoid cycles |
| [linked_nodes.ar](linked_nodes.ar) | A recursive struct linked by typed owning pointers |
| [collections.ar](collections.ar) | Maps, sets, linked lists, stacks, queues, and deques |
| [shared_collections.ar](shared_collections.ar) | Shared storage, explicit cloning, and weak collection observers |
| [debugging.ar](debugging.ar) | A small program for source breakpoints and stepping |

`argonc new native-demo --cpp` creates a complete additional example with an Argon caller and C++ header/source compiled together by the build tool. See [C++ interoperability](../docs/cpp.md).
