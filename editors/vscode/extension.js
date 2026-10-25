'use strict';
const vscode = require('vscode');
const { registerCommands } = require('./tools');
function activate(context) { registerCommands(vscode, context); }
function deactivate() {}
module.exports = { activate, deactivate };
