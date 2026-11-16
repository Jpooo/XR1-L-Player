const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('bridge', {
  call: (action, data) => ipcRenderer.invoke('voice-call', action, data),
  report: data => ipcRenderer.send('voice-status', data),
  ready: () => ipcRenderer.send('voice-ready'),
  onCommand: callback => ipcRenderer.on('voice-command', (_event, data) => callback(data)),
});
