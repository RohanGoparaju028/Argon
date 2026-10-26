---
title: VS Code
---

# Argon for VS Code

This extension provides `.ar` / `.fe` syntax highlighting, Run/Debug buttons, F5 debugging, file checks with clickable diagnostics, and project build/run/debug commands. Native debugging uses [CodeLLDB](https://github.com/vadimcn/codelldb), including its standard C++ value visualizers.

## Install

Build `argonc` from the repository first. Put it on PATH or set the VS Code setting `argon.compilerPath` to the executable's absolute path. If you open the compiler repository, the extension also finds `build/argonc` automatically.

Package the extension with Node.js and CMake (no npm downloads required):

```sh
cd editors/vscode
npm test
npm run package
```

In VS Code, use **Extensions: Install from VSIX** and select `argon-language-0.1.0.vsix`. Install CodeLLDB (`vadimcn.vscode-lldb`) if it is not already available; the extension declares it as an extension-pack companion. Running/checking do not require the debugger backend.

For development, launch VS Code with `--extensionDevelopmentPath=/absolute/path/to/Argon/editors/vscode`.

## Run and debug

Open a source file and use **Argon: Run File**, **Argon: Check File**, or **Argon: Debug File**. F5 and the editor-title buttons run the corresponding file actions. Commands save existing workspace buffers before compiling. Source imports are resolved by the compiler.

**Argon: Run Project**, **Build Project**, and **Debug Project** use the nearest `Argon.toml`, rather than treating an imported helper as the entry. These actions are also available from the Command Palette. The project root is used as the executable's working directory.

Debug builds use `argonc build --debug` and launch CodeLLDB after successful compilation. Breakpoints can be placed in `.ar` / `.fe` files. The compiler emits native debug information and source-line mappings; the debugger initially stops in the Argon `main` function. `argon.debugArgs` supplies executable arguments.

Runtime values are C++ representations: local identifiers include their source name, for example `ag_v3_answer`; struct/class fields currently use generated field indices. Shared and weak pointers and native container storage appear in the debugger's C++ views. Source-level expression evaluation in Argon syntax, custom pretty printers, a language server, and variable renaming are not implemented.

Workspace trust is required for checks as well as builds/runs: a check executes the source's global compile-time `test` blocks. The extension never runs compiler commands automatically when merely opening a file. Process arguments are passed without a shell.

See [CodeLLDB's manual](https://github.com/vadimcn/codelldb/blob/master/MANUAL.md) for debugger settings and platform setup.

## Current verification limits

Native source mappings and breakpoint resolution have been tested. Interactive stepping was not verified on the development host because LLDB could not pause the launched process. Debugging depends on your installed debugger and operating-system permissions. The Windows Vim integration has had a CI failure under investigation; it is not a verified platform guarantee. See [current status](status.md).
