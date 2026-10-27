# Copied native files

From the repository root:

```sh
./build/argonc run examples/native_files/src/main.ar
```

Output is `42`, `42`, `23`, and `9` on separate lines. The example uses a copied C++ implementation, a header-only function, a C11 implementation with C linkage, and `std::sqrt` from the system standard library. No downloads or external dependencies are needed.

Generated C++ and the executable appear in `argon-cxx/release/` under this example. To choose another location:

```sh
argonc run examples/native_files/src/main.ar --cxx-dir /absolute/path/to/output
```
