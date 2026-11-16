// Standalone fixture only: fake microphone flags never enter packaged main.cjs.
const { app, BrowserWindow, ipcMain, session } = require('electron');
app.commandLine.appendSwitch('use-fake-device-for-media-stream');
app.commandLine.appendSwitch('use-fake-ui-for-media-stream');
app.commandLine.appendSwitch('autoplay-policy', 'no-user-gesture-required');
app.commandLine.appendSwitch('disable-renderer-backgrounding');
app.commandLine.appendSwitch('disable-background-timer-throttling');
const path = require('node:path');
app.commandLine.appendSwitch('use-file-for-fake-audio-capture', path.join(__dirname, 'voice.wav'));
const assert = require('node:assert/strict');
app.setPath('userData', path.join(process.env.TMPDIR || '/tmp', 'xr1-webrtc-fixture'));
let source, peer, stage = 0, timer;
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
const invokePeer = (name, data) => peer.webContents.executeJavaScript(`window.${name}(${JSON.stringify(data)})`);
async function end(error) {
  clearTimeout(timer);
  if (error) { console.error(error); app.exit(1); }
  else { console.log('PASS hidden WebRTC: capture, SDP/ICE, microphone RTP, mute/unmute, stop and no visible window'); app.exit(0); }
}
app.whenReady().then(async () => {
  timer = setTimeout(() => end(new Error('WebRTC test timed out at stage ' + stage)), 40000);
  session.defaultSession.setPermissionRequestHandler((_web, permission, callback) => callback(permission === 'media'));
  session.defaultSession.setPermissionCheckHandler((_web, permission) => permission === 'media');
  source = new BrowserWindow({ show: false, webPreferences: { preload: path.join(__dirname, '../preload.cjs'), contextIsolation: true, nodeIntegration: false, sandbox: true, backgroundThrottling: false } });
  peer = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true, nodeIntegration: false, backgroundThrottling: false } });
  ipcMain.handle('voice-call', async (_event, action, data) => {
    assert.equal(data.key, 'fixture');
    if (action === 'sdp') return invokePeer('answer', data.sdp);
    if (action === 'ice') return invokePeer('candidates', data.candidates);
    throw new Error('Unexpected IPC request');
  });
  ipcMain.on('voice-status', async (_event, data) => {
    try {
      if (data.error) throw new Error(data.error);
      if (data.microphones && stage === 0) {
        assert.ok(data.microphones.length);
        stage = 1;
        source.webContents.send('voice-command', { action: 'start', key: 'fixture', deviceId: data.microphones[0].id });
      } else if (data.state === 'transmitting' && stage === 1) {
        assert.ok(data.packets > 0);
        assert.ok(await invokePeer('packetCount') > 0);
        assert.equal(source.isVisible(), false); assert.equal(peer.isVisible(), false);
        stage = 2;
        const receivedLevel = await invokePeer('audioLevel');
        if(receivedLevel<=.005)console.log('receive',JSON.stringify(await invokePeer('diagnostics')));
        assert.ok(receivedLevel > 0.005, 'no microphone sound reached the peer');
        source.webContents.send('voice-command', { action: 'mute', muted: true });
      } else if (data.state === 'muted' && stage === 2) {
        stage = 3;
        await pause(500);
        assert.ok(await invokePeer('audioLevel') < 0.001, 'microphone sound continued while muted');
        source.webContents.send('voice-command', { action: 'mute', muted: false });
      } else if (data.state === 'transmitting' && stage === 3) {
        stage = 4;
        assert.ok(await invokePeer('audioLevel') > 0.005, 'microphone sound did not return after unmute');
        source.webContents.send('voice-command', { action: 'stop' });
        await pause(1000);
        const before = await invokePeer('packetCount');
        await pause(1000);
        assert.equal(await invokePeer('packetCount'), before, 'audio packets continued after disconnect');
        end();
      }
    } catch (error) { end(error); }
  });
  ipcMain.once('voice-ready', () => source.webContents.send('voice-command', { action: 'devices' }));
  await peer.loadFile(path.join(__dirname, 'peer.html'));
  await source.loadFile(path.join(__dirname, '../renderer.html'));
}).catch(end);
