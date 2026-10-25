'use strict';
// A dependency-free VSIX archive. No registry access or publication is involved.
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');
const root = path.resolve(__dirname, '..');
const manifest = require(path.join(root, 'package.json'));
const stage = fs.mkdtempSync(path.join(os.tmpdir(), 'argon-vsix-'));
const output = path.join(root, `argon-language-${manifest.version}.vsix`);
try {
  fs.mkdirSync(path.join(stage, 'extension'));
  for (const name of ['package.json', 'extension.js', 'tools.js', 'language-configuration.json', 'syntaxes', 'README.md', 'LICENSE']) {
    fs.cpSync(path.join(root, name), path.join(stage, 'extension', name), { recursive: true });
  }
  fs.writeFileSync(path.join(stage, '[Content_Types].xml'), '<?xml version="1.0" encoding="utf-8"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="json" ContentType="application/json"/><Default Extension="js" ContentType="application/javascript"/><Default Extension="md" ContentType="text/markdown"/><Default Extension="vsixmanifest" ContentType="text/xml"/><Default Extension="txt" ContentType="text/plain"/><Override PartName="/extension/LICENSE" ContentType="text/plain"/></Types>');
  fs.writeFileSync(path.join(stage, 'extension.vsixmanifest'), `<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011"><Metadata><Identity Language="en-US" Id="${manifest.name}" Version="${manifest.version}" Publisher="${manifest.publisher}"/><DisplayName>Argon</DisplayName><Description xml:space="preserve">${manifest.description}</Description><Tags>argon,native</Tags><Categories>Programming Languages,Debuggers</Categories><Properties><Property Id="Microsoft.VisualStudio.Code.Engine" Value="${manifest.engines.vscode}"/></Properties></Metadata><Installation><InstallationTarget Id="Microsoft.VisualStudio.Code"/></Installation><Dependencies/><Assets><Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true"/></Assets></PackageManifest>`);
  const result = spawnSync('cmake', ['-E', 'tar', 'cf', output, '--format=zip', '[Content_Types].xml', 'extension.vsixmanifest', 'extension'], { cwd: stage, stdio: 'inherit' });
  if (result.error || result.status !== 0) throw result.error || new Error('VSIX packaging failed');
  console.log(output);
} finally { fs.rmSync(stage, { recursive: true, force: true }); }
