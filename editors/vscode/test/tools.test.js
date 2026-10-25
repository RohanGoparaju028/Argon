'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { registerCommands } = require('../tools');

function setup(t) {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'argon editor spaces '));
  t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  const source = path.join(directory, 'file $(literal).ar');
  fs.writeFileSync(source, 'fn main()->i32{return 0;}');
  const uri = { fsPath: source, scheme: 'file' };
  const folder = { uri: { fsPath: directory, scheme: 'file' } };
  const state = { commands: {}, tasks: [], errors: [], debug: [], builds: [], saves: 0 };
  const disposable = () => ({ dispose() {} });
  const vscode = {
    workspace: { isTrusted: true, workspaceFolders: [folder], getWorkspaceFolder: () => folder,
      getConfiguration: () => ({ get: (name, fallback) => name === 'compilerPath' ? '/compiler with spaces/argonc' : fallback }),
      saveAll: async () => { state.saves++; return true; },
      fs: { createDirectory: async value => fs.promises.mkdir(value.fsPath, { recursive: true }) } },
    window: { activeTextEditor: { document: { uri } }, showErrorMessage: message => state.errors.push(message),
      withProgress: async (_options, callback) => callback() },
    commands: { registerCommand: (name, callback) => { state.commands[name] = callback; return disposable(); } },
    tasks: { executeTask: async task => state.tasks.push(task) },
    Task: class { constructor(definition, scope, name, sourceName, execution) { Object.assign(this, { definition, scope, name, sourceName, execution }); } },
    ProcessExecution: class { constructor(command, args, options) { Object.assign(this, { command, args, options }); } },
    TaskScope: { Global: 1 }, TaskRevealKind: { Always: 1 }, ProgressLocation: { Notification: 1 },
    extensions: { getExtension: () => ({}) },
    debug: { startDebugging: async (_folder, config) => { state.debug.push(config); return true; }, onDidTerminateDebugSession: disposable }
  };
  const context = { subscriptions: [], globalStorageUri: { fsPath: path.join(directory, 'storage') } };
  const dependencies = { execute: async (...args) => { state.builds.push(args); return { stdout: '', stderr: '' }; } };
  registerCommands(vscode, context, dependencies);
  return { directory, source, vscode, state, dependencies };
}
test('file runner saves and uses process arguments, preserving spaces and shell characters', async t => {
  const { source, state } = setup(t);
  await state.commands['argon.runFile']();
  assert.equal(state.saves, 1);
  assert.equal(state.tasks[0].execution.command, '/compiler with spaces/argonc');
  assert.deepEqual(state.tasks[0].execution.args, ['run', source]);
  assert.equal(state.errors.length, 0);
});
test('workspace trust blocks all executable actions before saving or invoking tools', async t => {
  const { state, vscode } = setup(t);
  vscode.workspace.isTrusted = false;
  await state.commands['argon.runFile']();
  await state.commands['argon.debugFile']();
  await state.commands['argon.checkFile']();
  assert.equal(state.saves, 0);
  assert.equal(state.tasks.length + state.builds.length + state.debug.length, 0);
  assert.equal(state.errors.length, 3);
});
test('project runner discovers the manifest and lets the compiler select its entry', async t => {
  const { directory, state } = setup(t);
  fs.writeFileSync(path.join(directory, 'Argon.toml'), '[project]\n');
  await state.commands['argon.runProject']();
  assert.deepEqual(state.tasks[0].execution.args, ['run']);
  assert.equal(state.tasks[0].execution.options.cwd, directory);
});
test('debugger builds with source mappings before launching CodeLLDB', async t => {
  const { source, state } = setup(t);
  await state.commands['argon.debugFile']();
  assert.equal(state.errors.length, 0);
  const [compiler, args] = state.builds[0];
  assert.equal(compiler, '/compiler with spaces/argonc');
  assert.deepEqual(args.slice(0, 4), ['build', source, '--debug', '-o']);
  assert.equal(state.debug[0].program, args[4]);
  assert.equal(state.debug[0].type, 'lldb');
  assert.match(state.debug[0].preRunCommands[0], /ag_fn/);
});
test('missing CodeLLDB gives an actionable error and does not build', async t => {
  const { state, vscode } = setup(t);
  vscode.extensions.getExtension = () => undefined;
  await state.commands['argon.debugFile']();
  assert.match(state.errors[0], /Install CodeLLDB/);
  assert.equal(state.builds.length, 0);
});
test('a failed debug build never starts the debugger', async t => {
  const { state, vscode } = setup(t);
  vscode.window.withProgress = async () => { throw Object.assign(new Error('failed'), { stderr: 'type error' }); };
  await state.commands['argon.debugFile']();
  assert.equal(state.debug.length, 0);
  assert.match(state.errors[0], /type error/);
});
