---
title: Installation
---

# Install Argon

These instructions install the current **Argon 0.1.0 source build** on macOS, Linux, and Windows. Package managers install the prerequisites; CMake builds and installs `argonc` itself. After installation, `argonc` creates and builds your applications without a separate application CMake project.

Choose your operating system:

- [macOS](#macos): Apple Clang and CMake.
- [Linux](#linux): Ubuntu/Debian, Fedora, or Arch Linux with GCC and CMake.
- [Windows](#windows): native Windows builds using MSYS2 UCRT64.

## Requirements

| Requirement | Purpose |
| --- | --- |
| Git | Download and update the source repository |
| CMake 3.16 or newer | Configure, build, and install the compiler |
| A C++17-capable Clang or GCC toolchain | Build `argonc` and compile generated application code |
| Make on macOS/Linux, or Ninja in the Windows instructions | Execute the compiler's CMake build |

**Keep the native compiler installed after installing Argon.** `argonc build` and `argonc run` invoke it for every application build. `argonc check` does not require a native compiler. Python and Node.js are not required for the compiler; Node.js is only needed if you package the optional VS Code extension.

The commands use a separate `build-install/` directory so they can coexist with a development build in `build/`. A user-local prefix avoids needing administrator permission to install Argon itself. Installing system prerequisites may still require it.

## macOS

Use Terminal with zsh or Bash. The native build targets the architecture of your selected toolchain; these commands do not create a universal binary.

### 1. Install prerequisites

Install Apple's Command Line Tools if they are not already installed, and finish the installer dialog before continuing:

```sh
xcode-select --install
```

If the tools are already installed, skip that command. They provide Apple Clang, Git, and Make. See [Apple's Command Line Tools installation guide](https://developer.apple.com/documentation/xcode/installing-the-command-line-tools/).

Install [Homebrew](https://brew.sh/) if needed, following its PATH setup instructions, then install CMake:

```sh
brew install cmake
```

This installs the [CMake prerequisite](https://formulae.brew.sh/formula/cmake), not an Argon package. Check that the tools are available:

```sh
git --version
cmake --version
clang++ --version
```

### 2. Download, build, and install

```sh
git clone https://github.com/RohanGoparaju028/Argon.git
cd Argon
cmake -S . -B build-install -DCMAKE_BUILD_TYPE=Release
cmake --build build-install --parallel 2
cmake --install build-install --prefix "$HOME/.local"
```

If you already have a checkout, open its directory and start with the CMake commands. Do not clone another copy unless you want one.

### 3. Add Argon to PATH

Add this line once to `~/.zshrc` for the default macOS zsh shell:

```sh
export PATH="$HOME/.local/bin:$PATH"
```

Reload the configuration:

```sh
source ~/.zshrc
argonc --version
argonc doctor
```

For Bash, put the same export line in `~/.bash_profile` and reload that file instead. To enable the current terminal immediately without editing a file, run the export command directly; that change lasts only for that terminal session.

`argonc --version` should print `Argon 0.1.0`; `argonc doctor` should print the path to a C++ compiler. Continue with [your first installed project](#verify-the-installation-with-a-project).

## Linux

Use Bash or zsh. Install prerequisites for your distribution, then use the common build steps below.

### 1. Install prerequisites

**Ubuntu / Debian:**

```sh
sudo apt update
sudo apt install git cmake build-essential
```

`build-essential` supplies the compiler/build tools; see the [Ubuntu package information](https://packages.ubuntu.com/jammy/build-essential).

**Fedora:**

```sh
sudo dnf install git cmake gcc-c++ make
```

`gcc-c++` supplies GCC's C++ support; see the [Fedora package information](https://packages.fedoraproject.org/pkgs/gcc/gcc-c%2B%2B/).

**Arch Linux:**

```sh
sudo pacman -Syu --needed git cmake base-devel
```

Use a full system update with the installation. The [base-devel package](https://archlinux.org/packages/core/any/base-devel/) supplies the development tools.

For other distributions, install Git, CMake 3.16+, Make, and a C++17-capable GCC/Clang toolchain with its C++ standard-library development files. Distribution package names may differ. Very old distributions may need a newer toolchain than their default packages provide.

Verify the tools:

```sh
git --version
cmake --version
g++ --version
```

### 2. Download, build, and install

```sh
git clone https://github.com/RohanGoparaju028/Argon.git
cd Argon
cmake -S . -B build-install -DCMAKE_BUILD_TYPE=Release
cmake --build build-install --parallel 2
cmake --install build-install --prefix "$HOME/.local"
```

For an existing checkout, start with the CMake commands from its root directory. The user-local install does not need `sudo`.

### 3. Add Argon to PATH

Add this line once to `~/.bashrc` for Bash, or `~/.zshrc` for zsh:

```sh
export PATH="$HOME/.local/bin:$PATH"
```

For Bash, reload and verify:

```sh
source ~/.bashrc
argonc --version
argonc doctor
```

For zsh, use `source ~/.zshrc` instead. A terminal-only setup can run the export command directly. Continue with [your first installed project](#verify-the-installation-with-a-project).

## Windows

The documented native Windows route uses **MSYS2 UCRT64 on x86-64 Windows**, with MinGW-w64 GCC and the native MinGW CMake/Ninja packages. These are Bash commands to run in the **MSYS2 UCRT64 terminal**, not PowerShell or Command Prompt. MSVC `cl.exe` is not a supported backend for Argon application builds.

### 1. Install MSYS2 and prerequisites

Install MSYS2 using its [official getting-started instructions](https://www.msys2.org/). Open **MSYS2 UCRT64** from the Start menu. This environment puts the matching native tools and runtime libraries on PATH; see [MSYS2 environments](https://www.msys2.org/docs/environments/).

Update packages:

```sh
pacman -Syu
```

If the update asks to close the terminal, finish that step, reopen **MSYS2 UCRT64**, and run `pacman -Syu` again. Follow [MSYS2's update instructions](https://www.msys2.org/docs/updating/) until the update is complete.

Install the tools:

```sh
pacman -S --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

Use the UCRT64 CMake package rather than the plain MSYS `cmake` package when building native Windows programs. The build below explicitly selects Ninja, following the [MSYS2 CMake guide](https://www.msys2.org/docs/cmake/).

Check the active environment and tools:

```sh
echo "$MSYSTEM"
g++ --version
cmake --version
ninja --version
```

`MSYSTEM` must print `UCRT64`.

### 2. Download, build, and install

Run these commands in that same UCRT64 terminal:

```sh
git clone https://github.com/RohanGoparaju028/Argon.git
cd Argon
cmake -S . -B build-install -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build-install --parallel 2
cmake --install build-install --prefix "$(cygpath -m "$HOME/.local")"
```

`cygpath` converts the MSYS2 home path to a native Windows path for CMake's installation prefix. The executable is installed as `argonc.exe` under `$HOME/.local/bin` in your MSYS2 home directory.

### 3. Add Argon to PATH and select GCC

Add these lines once to the MSYS2 user's `~/.bashrc`:

```sh
export PATH="$HOME/.local/bin:$PATH"
export ARGON_CXX=g++
```

Then reload and verify:

```sh
source ~/.bashrc
argonc --version
argonc doctor
```

`ARGON_CXX=g++` selects the UCRT64 compiler for application builds, rather than an unrelated Clang installation inherited from Windows PATH. Keep using the UCRT64 terminal so the matching MinGW runtime DLLs remain available. Copying only `argonc.exe` to another machine does not bundle those DLLs or the native compiler.

### Using Windows editors

Launch your editor from the configured UCRT64 environment when possible, or configure its environment with both the installed Argon `bin` directory and the MSYS2 `ucrt64/bin` directory. The editor needs access to the executable, native compiler, and runtime DLLs.

Find the native Windows path to the installed compiler with:

```sh
cygpath -w "$HOME/.local/bin/argonc.exe"
```

Use that absolute path for `argon.compilerPath` in VS Code or `g:argon_compiler` in Vim/Neovim. See the [VS Code guide](vscode.md) and [Vim/Neovim guide](vim.md) for setup. Editor plugins are optional and are not activated automatically by installing the compiler.

If using WSL instead, follow the [Linux instructions](#linux) inside your WSL distribution. That produces Linux binaries; it is a separate setup from the native Windows compiler above.

## Verify the installation with a project

Run these commands in your configured macOS/Linux shell or Windows UCRT64 terminal:

```sh
argonc new "$HOME/argon-install-demo" --cpp
cd "$HOME/argon-install-demo"
argonc check
argonc run
```

Use a different project name if that directory already exists. `argonc run` should print:

```text
42
```

This checks more than whether the executable starts: it creates a project, compiles Argon, compiles the generated C++ and the local `cpp/demo.cpp`, links them, and executes the result.

The project contains `src/main.ar`, `cpp/demo.hpp`, `cpp/demo.cpp`, and `Argon.toml`. Generated output is in `argon-cxx/release/`, with an `.exe` executable suffix on Windows. Edit `src/main.ar` and run again. See [Getting started](getting-started.md) for a first Argon program and [C++ interoperability](cpp.md) for native libraries.

## What gets installed

For an installation prefix such as `$HOME/.local`:

| Location under the prefix | Contents |
| --- | --- |
| `bin/argonc` or `bin/argonc.exe` | Compiler and integrated project builder |
| `share/doc/argon/` | README, license, language documentation, and examples |
| `share/argon/editors/` | Vim/Neovim plugin and VS Code extension sources |

The Argon runtime source is embedded in the compiler; a separate runtime-header installation is not required. Native compiler/toolchain libraries and third-party C++ dependencies are still supplied by your environment.

To install elsewhere, replace the `--prefix` argument and add that prefix's `bin` directory to PATH. CMake's [installation options](https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project) describe how the prefix works. Prefer a user-writable location unless you deliberately want a system-wide install.

## Update an existing installation

From the original source checkout, with your local changes committed or otherwise preserved:

```sh
git pull --ff-only origin main
cmake --build build-install --parallel 2
```

Then reinstall using the same prefix as before. On macOS/Linux:

```sh
cmake --install build-install --prefix "$HOME/.local"
```

On Windows, in UCRT64:

```sh
cmake --install build-install --prefix "$(cygpath -m "$HOME/.local")"
```

Verify with `argonc --version` and `argonc doctor`. The current development compiler may retain the same version string across commits; `git rev-parse --short HEAD` identifies the source revision you built. `git pull --ff-only` stops on divergent local history instead of silently creating a merge.

## Optional compiler tests

From the source checkout, after building:

```sh
cd build-install
ctest --output-on-failure
cd ..
```

On Windows, run this in UCRT64 with `ARGON_CXX=g++` set as above. Extra editor/debugger tests are discovered when the corresponding tools are installed, so test counts vary. These tests are not required just to invoke the installed compiler.

The repository's [compiler CI](https://github.com/RohanGoparaju028/Argon/actions/workflows/ci.yml) builds/tests on Ubuntu, macOS, and Windows UCRT64. The installation workflow has additionally been smoke-tested locally on macOS; this does not certify every Linux distribution or CPU architecture.

## Troubleshooting

| Symptom | Resolution |
| --- | --- |
| `argonc: command not found` | Confirm installation succeeded and the chosen prefix's `bin` directory is on PATH. Reload the shell configuration or open a new terminal. |
| An older `argonc` runs | Use `command -v argonc` to identify the executable; put the intended installation first on PATH. |
| CMake cannot find a C++ compiler | Install the OS toolchain above. On Windows, use UCRT64 and explicitly select `g++`. |
| CMake reports a generator/compiler mismatch | Use a fresh build directory, such as `build-install-new`, when changing generators or toolchains. Do not reuse a cache created for another environment. |
| `argonc doctor` cannot find a compiler | Keep Clang/GCC installed and on PATH, or set `ARGON_CXX` to one compiler executable path. It cannot be a command with extra flags. |
| Windows reports missing DLLs | Run in UCRT64 or ensure its `ucrt64/bin` is on the launching process's PATH. |
| Windows finds the wrong `clang++` | Set `ARGON_CXX=g++` in UCRT64, then run `argonc doctor`. |
| Installation reports permission denied | Use the user-local prefix above, or another writable directory. |
| `new` reports the destination already exists | Choose a new directory name, or use `argonc init` in an existing directory without conflicting generated files. |
| A C++ library header/link fails | Install or copy the library's actual dependencies; Argon installation does not install arbitrary native libraries. See [native integration](cpp.md). |

Once `argonc run` prints `42` for the native demo, continue with the [keyword reference](keywords.md), [language reference](language.md), and [build guide](build.md).
