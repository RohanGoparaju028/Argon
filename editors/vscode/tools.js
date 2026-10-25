'use strict';
const fs = require('fs');
const path = require('path');
const { promisify } = require('util');
const execute = promisify(require('child_process').execFile);

function findProject(start) {
  let current = path.resolve(start);
  while (true) {
    if (fs.existsSync(path.join(current, 'Argon.toml'))) return current;
    const parent = path.dirname(current);
    if (parent === current) return undefined;
    current = parent;
  }
}
function compilerPath(vscode, uri) {
  const configured = vscode.workspace.getConfiguration('argon', uri).get('compilerPath');
  if (configured) return configured;
  const name = process.platform === 'win32' ? 'argonc.exe' : 'argonc';
  const folder = uri && vscode.workspace.getWorkspaceFolder(uri);
  return [folder && path.join(folder.uri.fsPath, 'build', name), path.resolve(__dirname, '../../build', name)]
    .find(candidate => candidate && fs.existsSync(candidate)) || name;
}
function registerCommands(vscode, context, dependencies = {}) {
  const execFile = dependencies.execute || execute;
  let building = false;
  async function launch(action, projectMode) {
    try {
      if (!vscode.workspace.isTrusted) throw new Error('Trust this workspace before checking, running, or debugging Argon code.');
      const doc = vscode.window.activeTextEditor?.document;
      const usable = doc && doc.uri.scheme === 'file' && ['.ar', '.fe'].includes(path.extname(doc.uri.fsPath));
      if (!projectMode && !usable) throw new Error('Open a saved .ar or .fe source file first.');
      if (!await vscode.workspace.saveAll(false)) return;
      const uri = usable ? doc.uri : vscode.workspace.workspaceFolders?.[0]?.uri;
      if (!uri) throw new Error('Open an Argon source file or project folder.');
      const folder = vscode.workspace.getWorkspaceFolder(uri);
      const project = findProject(usable ? path.dirname(uri.fsPath) : uri.fsPath);
      if (projectMode && !project) throw new Error('No Argon.toml found. Create a project with argonc new NAME.');
      const cwd = project || path.dirname(uri.fsPath);
      const sourceArgs = projectMode ? [] : [uri.fsPath];
      const compiler = compilerPath(vscode, uri);
      if (action !== 'debug') {
        const task = new vscode.Task({ type: 'argon', action, file: uri.fsPath }, folder || vscode.TaskScope.Global,
          `${action} Argon`, 'Argon', new vscode.ProcessExecution(compiler, [action, ...sourceArgs], { cwd }), ['$argon', '$gcc']);
        task.presentationOptions = { reveal: vscode.TaskRevealKind.Always, focus: action === 'run' };
        await vscode.tasks.executeTask(task);
        return;
      }
      if (!vscode.extensions.getExtension('vadimcn.vscode-lldb')) throw new Error('Install CodeLLDB (vadimcn.vscode-lldb) to debug native Argon programs.');
      if (building) throw new Error('An Argon debug build is already in progress.');
      building = true;
      try {
        const storage = context.storageUri || context.globalStorageUri;
        await vscode.workspace.fs.createDirectory(storage);
        const directory = await fs.promises.mkdtemp(path.join(storage.fsPath, 'argon-debug-'));
        const binary = path.join(directory, process.platform === 'win32' ? 'program.exe' : 'program');
        try {
          await vscode.window.withProgress({ location: vscode.ProgressLocation.Notification, title: 'Building Argon for debugging' }, async () => {
            await execFile(compiler, ['build', ...sourceArgs, '--debug', '-o', binary], { cwd, maxBuffer: 8 * 1024 * 1024 });
          });
          const config = { type: 'lldb', request: 'launch', name: 'Debug Argon', program: binary, cwd,
            args: vscode.workspace.getConfiguration('argon', uri).get('debugArgs', []), sourceLanguages: ['cpp'],
            stopOnEntry: false, preRunCommands: ['breakpoint set --func-regex "^ag_fn[0-9]+_main[(][)]$"'] };
          if (!await vscode.debug.startDebugging(folder, config)) throw new Error('CodeLLDB could not start the Argon debug session.');
          const cleanup = vscode.debug.onDidTerminateDebugSession(session => {
            if (session.configuration.program === binary) {
              fs.promises.rm(directory, { recursive: true, force: true }).catch(() => {});
              cleanup.dispose();
            }
          });
          context.subscriptions.push(cleanup);
        } catch (error) {
          await fs.promises.rm(directory, { recursive: true, force: true });
          throw error;
        }
      } finally { building = false; }
    } catch (error) { vscode.window.showErrorMessage(`Argon: ${error.stderr || error.message}`); }
  }
  for (const [name, action, project] of [
    ['runFile', 'run', false], ['debugFile', 'debug', false], ['checkFile', 'check', false],
    ['runProject', 'run', true], ['debugProject', 'debug', true], ['buildProject', 'build', true]
  ]) context.subscriptions.push(vscode.commands.registerCommand(`argon.${name}`, () => launch(action, project)));
}
module.exports = { registerCommands, findProject, compilerPath };
