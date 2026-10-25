local M = {}
-- Optional nvim-dap integration. Without a configured adapter, ArgonDebug
-- falls back to a native LLDB terminal in the Vimscript frontend.
function M.debug(target)
  local ok, dap = pcall(require, 'dap')
  if not ok then return false end
  local adapter = vim.g.argon_dap_command or 'lldb-dap'
  if vim.fn.executable(adapter) ~= 1 then return false end
  dap.adapters.argon = { type = 'executable', command = adapter, name = 'Argon LLDB' }
  dap.run({
    type = 'argon', request = 'launch', name = 'Debug Argon',
    program = target.program, cwd = target.cwd, args = {}, stopOnEntry = false,
    preRunCommands = { 'breakpoint set --func-regex "^ag_fn[0-9]+_main[(][)]$"' },
  })
  return true
end
return M
