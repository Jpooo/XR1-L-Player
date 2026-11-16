const { app, BrowserWindow, ipcMain, session, safeStorage } = require('electron');
const fs = require('node:fs');
const path = require('node:path');
const https = require('node:https');
const { Msal, TokenStore } = require('xal-node');
const WebApi = require('xbox-webapi').default;
const { XHome, delay } = require('./xhome.cjs');
const { writeState, readCommands, cleanError } = require('./protocol.cjs');
const { pollDeviceCode } = require('./auth.cjs');

// Third-party auth/client debug output can contain account tokens. Never log it.
console.log = () => {};
console.debug = () => {};
const originalRequest = https.request;
https.request = function (...args) {
  const request = originalRequest.apply(this, args);
  request.setTimeout(15000, () => request.destroy(new Error('Xbox request timed out.')));
  return request;
};
const argument = key => { const i = process.argv.indexOf(key); return i < 0 ? '' : process.argv[i + 1] || ''; };
const dir = argument('--bridge-dir');
const owner = Number(argument('--owner-pid'));
if (!path.isAbsolute(dir) || !fs.existsSync(dir) || !Number.isSafeInteger(owner) || owner < 1) process.exit(2);
app.setPath('userData', path.join(app.getPath('appData'), 'XR1-L Player Microphone Experimental'));
app.commandLine.appendSwitch('autoplay-policy', 'no-user-gesture-required');
app.commandLine.appendSwitch('disable-renderer-backgrounding');
app.commandLine.appendSwitch('disable-background-timer-throttling');
let window, auth, tokenStore, api, consoles = [], microphones = [], epoch = 0, signingIn = false, quitting = false, rendererReady;
let keepalive, keepaliveBusy = false, sessionKey = '';
const state = { state: 'starting', message: 'Starting microphone connection...', code: '', account: 'Signed out', packets: '0', remembered: 'false' };
const publish = patch => {
  Object.assign(state, patch);
  try { writeState(dir, state); } catch (error) { if (!['EBUSY', 'EPERM', 'EACCES'].includes(error.code) && !quitting) app.quit(); }
};
function saveChoices(choices) {
  const target = path.join(app.getPath('userData'), 'devices.json');
  fs.writeFileSync(target + '.tmp', JSON.stringify(choices), { mode: 0o600 });
  fs.renameSync(target + '.tmp', target);
}
function choices() {
  try { return JSON.parse(fs.readFileSync(path.join(app.getPath('userData'), 'devices.json'), 'utf8')); }
  catch { return {}; }
}
function updateList(kind, values) {
  for (const key of Object.keys(state)) if (key.startsWith(kind + '.')) delete state[key];
  values.forEach((value, i) => { state[`${kind}.${i}`] = value.name; });
  const selected = choices()[kind];
  publish({ revision: String(Number(state.revision || 0) + 1), [`${kind}Count`]: String(values.length), [`${kind}Selected`]: String(Math.max(0, values.findIndex(v => v.id === selected))) });
}
class EncryptedStore extends TokenStore {
  constructor() {
    super();
    this.file = path.join(app.getPath('userData'), 'account.enc');
    this.canPersist = safeStorage.isEncryptionAvailable() && (process.platform !== 'linux' || safeStorage.getSelectedStorageBackend() !== 'basic_text');
    if (this.canPersist) {
      try { this.loadJson(safeStorage.decryptString(fs.readFileSync(this.file))); } catch { /* signed out */ }
    }
  }
  save() {
    if (!this.canPersist || !this._userToken) return;
    const encrypted = safeStorage.encryptString(JSON.stringify({ userToken: this._userToken.data }));
    fs.writeFileSync(this.file + '.tmp', encrypted, { mode: 0o600 });
    fs.renameSync(this.file + '.tmp', this.file);
  }
  removeAll() {
    this._userToken = undefined; this._sisuToken = undefined; this._jwtKeys = undefined;
    fs.rmSync(this.file, { force: true });
  }
}
async function refreshConsoles(own = epoch) {
  const token = await auth.getWebToken();
  const web = new WebApi({ uhs: token.data.DisplayClaims.xui[0].uhs, token: token.data.Token });
  const result = await web.providers.smartglass.getConsolesList();
  if (own !== epoch) return;
  consoles = (result.data?.result || []).map(c => ({ id: c.id || c.serverId, name: c.name || c.consoleName || 'Xbox console' })).filter(c => c.id).slice(0, 32);
  updateList('console', consoles);
  publish({ state: 'ready', account: 'Signed in', code: '', message: consoles.length ? 'Choose your console and microphone, then Connect.' : 'No consoles found. Enable Remote features on your Xbox, then Refresh.' });
}
async function signIn(interactive = false) {
  if (signingIn) return;
  signingIn = true;
  const own = ++epoch;
  await disconnect(false);
  try {
    if (interactive) { tokenStore.removeAll(); auth = new Msal(tokenStore); }
    publish({ state: 'signing-in', message: 'Requesting Microsoft sign-in code...', code: '' });
    if (!tokenStore.getUserToken()) {
      const device = await auth.doDeviceCodeAuth();
      if (own !== epoch) return;
      publish({ code: device.user_code, message: 'Open Microsoft sign-in and enter the code below. You can use any browser or your phone.' });
      await pollDeviceCode(auth, tokenStore, device, () => own === epoch && !quitting);
    }
    if (own !== epoch) return;
    await refreshConsoles(own);
    publish({ remembered: String(tokenStore.canPersist) });
  } catch (error) {
    if (own === epoch) publish({ state: 'error', message: cleanError(error), code: '', account: 'Sign in again' });
  } finally { signingIn = false; }
}
async function disconnect(show = true) {
  sessionKey = '';
  clearInterval(keepalive);
  if (window && !window.isDestroyed()) window.webContents.send('voice-command', { action: 'stop' });
  const previous = api; api = undefined;
  if (show) publish({ state: 'ready', message: 'Microphone disconnected. Game playback is unchanged.', packets: '0', code: '' });
  if (previous) await previous.stop();
}
async function connect(consoleIndex, micIndex) {
  if (signingIn) throw new Error('Finish Microsoft sign-in first.');
  const target = consoles[consoleIndex], microphone = microphones[micIndex];
  if (!target) throw new Error('Choose an Xbox console first.');
  if (!microphone) throw new Error('Choose a PC microphone first.');
  const own = ++epoch;
  await disconnect(false);
  if (own !== epoch) return;
  publish({ state: 'connecting', message: 'Connecting microphone to Xbox...', packets: '0' });
  const gssv = await auth.getGssvToken();
  const token = await auth.getStreamToken(gssv.data.Token, 'xhome');
  if (own !== epoch) return;
  const region = token.getDefaultRegion() || token.getRegions()[0];
  if (!region?.baseUri) throw new Error('No Xbox Remote Play region available.');
  const current = new XHome(region.baseUri, token.data.gsToken);
  api = current;
  try {
    await current.start(target.id);
    const deadline = Date.now() + 90000;
    let provisioned = false;
    while (Date.now() < deadline && own === epoch) {
      const result = await current.state();
      if (result.state === 'Provisioned') { provisioned = true; break; }
      if (result.state === 'ReadyToConnect') await current.connect((await auth.getMsalToken()).data.lpt);
      else if (!['Provisioning', 'WaitingForResources', 'ReadyToConnect'].includes(result.state)) throw new Error('Xbox could not start Remote Play. Check Remote features and console power.');
      await delay(1000);
    }
    if (own !== epoch) { await current.stop(); return; }
    if (!provisioned) throw new Error('Xbox connection timed out. Check Remote features and console power.');
    await rendererReady;
    if (own !== epoch) return;
    sessionKey = String(own);
    window.webContents.send('voice-command', { action: 'start', deviceId: microphone.id, key: sessionKey });
    saveChoices({ console: target.id, mic: microphone.id });
    keepalive = setInterval(async () => {
      if (keepaliveBusy || own !== epoch) return;
      keepaliveBusy = true;
      try { await current.keepalive(); }
      catch (error) { if (own === epoch) await fail(error); }
      finally { keepaliveBusy = false; }
    }, 25000);
  } catch (error) { void current.stop(); if (api === current) api = undefined; if (own === epoch) throw error; }
}
async function fail(error) {
  ++epoch;
  publish({ state: 'error', message: cleanError(error), packets: '0' });
  await disconnect(false);
}
app.whenReady().then(async () => {
  fs.mkdirSync(app.getPath('userData'), { recursive: true, mode: 0o700 });
  const ses = session.fromPartition('persist:xr1-microphone');
  const permitted = (web, permission, details) => permission === 'media' && web === window?.webContents && web.getURL().startsWith('file://') && !(details?.mediaTypes || []).includes('video');
  ses.setPermissionCheckHandler((web, permission, _origin, details) => permitted(web, permission, details));
  ses.setPermissionRequestHandler((web, permission, callback, details) => callback(permitted(web, permission, details)));
  ses.setDevicePermissionHandler(details => details.deviceType === 'audio');
  window = new BrowserWindow({ show: false, width: 2, height: 2, skipTaskbar: true, webPreferences: { partition: 'persist:xr1-microphone', preload: path.join(__dirname, 'preload.cjs'), nodeIntegration: false, contextIsolation: true, sandbox: true, backgroundThrottling: false, autoplayPolicy: 'no-user-gesture-required' } });
  window.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
  window.webContents.on('will-navigate', event => event.preventDefault());
  rendererReady = new Promise(resolve => ipcMain.once('voice-ready', event => { if (event.sender === window.webContents) resolve(); }));
  ipcMain.handle('voice-call', async (event, action, data) => {
    if (event.sender !== window.webContents || !api) throw new Error('No active Xbox connection.');
    if (data.key !== sessionKey) throw new Error('Connection was cancelled.');
    if (action === 'sdp' && typeof data.sdp === 'string' && data.sdp.length < 100000) return api.sdp(data.sdp, data.chat === true);
    if (action === 'ice' && Array.isArray(data.candidates) && data.candidates.length < 128) return api.ice(data.candidates);
    throw new Error('Invalid microphone command.');
  });
  ipcMain.on('voice-status', (event, data) => {
    if (event.sender !== window.webContents || !data || typeof data !== 'object') return;
    if (Array.isArray(data.microphones)) {
      microphones = data.microphones.filter(d => typeof d.id === 'string' && typeof d.name === 'string').slice(0, 64);
      updateList('mic', microphones);
    }
    if (data.key !== sessionKey || !sessionKey) { if (data.error && !data.key && !api) publish({ state: 'error', message: cleanError(new Error(data.error)) }); return; }
    if (data.error) { void fail(new Error(data.error)); return; }
    if (['transmitting', 'waiting', 'muted'].includes(data.state)) publish({ state: data.state, message: String(data.detail || '').slice(0, 240), packets: String(data.packets || '0') });
  });
  window.webContents.on('render-process-gone', () => { void fail(new Error('The microphone process stopped. Close and reopen Xbox microphone settings.')); });
  tokenStore = new EncryptedStore(); auth = new Msal(tokenStore);
  await window.loadFile(path.join(__dirname, 'renderer.html'));
  publish({ state: 'idle', message: 'Sign in to Xbox, then choose your console and microphone.' });
  setInterval(() => { if (!quitting) publish({}); }, 1000);
  setInterval(() => {
    try { process.kill(owner, 0); } catch { app.quit(); return; }
    if (!fs.existsSync(dir)) { app.quit(); return; }
    let commands;
    try { commands = readCommands(dir); } catch { return; }
    for (const [command, ...args] of commands) {
      if (command === 'quit') { app.quit(); break; }
      if (command === 'signin') void signIn(true);
      else if (command === 'devices') void rendererReady.then(() => window.webContents.send('voice-command', { action: 'devices' }));
      else if (command === 'refresh') { if (!api && !signingIn) void refreshConsoles().catch(fail); }
      else if (command === 'connect' && state.state !== 'connecting') {
        if (String(args[2]) !== String(state.revision || 0)) publish({ message: 'The device list changed. Choose the devices again.' });
        else void connect(Number(args[0]), Number(args[1])).catch(fail);
      }
      else if (command === 'stop') { ++epoch; void disconnect(); }
      else if (command === 'mute' && api) window.webContents.send('voice-command', { action: 'mute', muted: args[0] === '1' });
      else if (command === 'signout') { ++epoch; void disconnect(false); tokenStore.removeAll(); auth = new Msal(tokenStore); consoles = []; updateList('console', consoles); publish({ state: 'idle', account: 'Signed out', code: '', message: 'Signed out. Microphone disconnected.' }); }
    }
  }, 100);
  if (tokenStore.getUserToken()) void signIn();
}).catch(error => { publish({ state: 'error', message: cleanError(error) }); app.quit(); });
app.on('before-quit', event => {
  if (quitting) return;
  event.preventDefault(); quitting = true; ++epoch;
  const shutdown = disconnect(false);
  Promise.race([shutdown, delay(1500)]).finally(() => { window?.destroy(); app.exit(0); });
});
process.on('unhandledRejection', error => { void fail(error); });
