---
title: Vim and Neovim
---

# Argon in Vim and Neovim

The same runtime plugin supports both editors: file detection, syntax highlighting, indentation/comment settings, checks with quickfix diagnostics, project/file running, and native debugging.

## Install

Add the repository’s `editors/vim` directory to your runtime path before enabling filetype plugins:

```vim
set runtimepath+=/absolute/path/to/Argon/editors/vim
filetype plugin on
syntax on
let g:argon_compiler = '/absolute/path/to/Argon/build/argonc'
```

For Neovim Lua configuration:

```lua
vim.opt.rtp:append('/absolute/path/to/Argon/editors/vim')
vim.g.argon_compiler = '/absolute/path/to/Argon/build/argonc'
vim.cmd('filetype plugin on')
vim.cmd('syntax on')
```

You can instead install this directory as a normal Vim/Neovim package. If no compiler is configured, the plugin searches ancestor `build/argonc` paths, then PATH. Windows uses `argonc.exe`.

## Commands

| Command | Action |
| --- | --- |
| `:ArgonRun` | Build and run the current file in a terminal split |
| `:ArgonRunProject` | Build and run the nearest manifest's entry |
| `:ArgonCheck` | Check the file and execute its compile-time blocks |
| `:ArgonCheckProject` | Check the manifest's entry |
| `:ArgonBuild` | Build the project |
| `:ArgonDebug` | Debug the current file |
| `:ArgonDebugProject` | Debug the project |

Commands save the current buffer; save modified imported modules before invoking a project command. Build/check errors populate quickfix and open it on failure. Use `:cnext`, `:cprev`, and `:copen` to navigate. Optional mappings can use `nnoremap <buffer> <F5> :ArgonDebug<CR>` and `<F6>` for `:ArgonRun`.

## Debugging

Vim needs terminal support (`+terminal`) and LLDB. Neovim also supports this terminal workflow without additional plugins. `:ArgonDebug` first builds an unoptimized executable with source mappings, then opens LLDB, sets a breakpoint in the Argon main function, and runs to it. Commands such as `breakpoint set --file pointers.ar --line 10`, `next`, `step`, `frame variable`, and `continue` operate on the native program. Set `g:argon_lldb` for a non-default LLDB executable. Plugin-owned temporary debug files are removed when the editor exits.

Neovim optionally uses [nvim-dap](https://github.com/mfussenegger/nvim-dap) when it is installed and an `lldb-dap` executable is available. Set:

```lua
vim.g.argon_dap_command = '/absolute/path/to/lldb-dap'
```

Then `:ArgonDebug` uses DAP instead of a terminal debugger. Use nvim-dap's `toggle_breakpoint()`, `continue()`, `step_over()`, and `step_into()` functions and your preferred DAP UI. There is no bundled adapter or automatic debugger download. The default adapter command is `lldb-dap`; on macOS, `xcrun -f lldb-dap` can locate it. See the [official LLDB DAP guide](https://lldb.llvm.org/use/lldbdap.html).

Debuggers show generated C++ representations, including source names within generated local identifiers. There is no Argon expression evaluator or custom variable formatter yet. Build/run/check only happen on explicit commands, not file-opening or save autocommands.

## Current verification limits

Native source mappings and breakpoint resolution have been tested. Interactive stepping was not verified on the development host because LLDB could not pause the launched process. Debugging depends on your installed debugger and operating-system permissions. The Windows Vim integration has had a CI failure under investigation; it is not a verified platform guarantee. See [current status](status.md).
