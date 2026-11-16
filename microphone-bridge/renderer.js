import Player from 'xbox-xcloud-player';

let player, microphone, generation = 0, statsTimer, statsBusy = false, previousPackets = 0;
let selectedDevice = '', muted = false, lastAdvance = 0, connectedAt = 0, sessionKey = '', actionEpoch = 0;
const bridge = window.bridge;
const report = data => bridge.report({ ...data, key: sessionKey });
const reportError = error => report({ error: String(error?.message || error).slice(0, 200) });
const noInput = { setApplication() {}, start() {}, stop() {}, run() {}, requestStates() { return []; } };
async function createOffer(p) {
  const offer = await p._webrtcClient.createOffer();
  offer.sdp = p._setBitrate(offer.sdp, 'video', 512);
  await p._webrtcClient.setLocalDescription(offer);
  return offer;
}

async function listMicrophones() {
  // Labels require permission. Release this probe immediately, before streaming.
  const probe = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
  probe.getTracks().forEach(track => track.stop());
  const devices = (await navigator.mediaDevices.enumerateDevices()).filter(d => d.kind === 'audioinput');
  bridge.report({ microphones: devices.map(d => ({ id: d.deviceId, name: d.label || 'Microphone' })) });
}
async function stop() {
  ++generation;
  clearInterval(statsTimer);
  microphone?.getTracks().forEach(track => track.stop());
  microphone = undefined;
  if (player) { try { player.close(); } catch {} }
  player = undefined;
  document.getElementById('remote').replaceChildren();
  previousPackets = 0; lastAdvance = 0;
}
async function start(command) {
  await stop();
  const own = generation;
  sessionKey = command.key;
  selectedDevice = command.deviceId || '';
  muted = false;
  const stream = await navigator.mediaDevices.getUserMedia({ audio: { channelCount: 1, sampleRate: 24000, echoCancellation: true, noiseSuppression: true, ...(selectedDevice ? { deviceId: { exact: selectedDevice } } : {}) }, video: false });
  if (own !== generation) { stream.getTracks().forEach(track => track.stop()); return; }
  microphone = stream;
  for (const track of stream.getAudioTracks()) track.onended = () => { reportError(new Error('Microphone disconnected. Choose it again and reconnect.')); void stop(); };
  player = new Player('remote', { input_driver: noInput, input_legacykeyboard: false, input_touch: false, input_mousekeyboard: false });
  player.bind();
  const p = player;
  // Never poll a PC gamepad or forward keys; control stays with the console.
  p._keyboardDriver.start = () => {};
  p._keyboardDriver.stop = () => {};
  // Retain the protocol's controller-presence handshake; never forward any
  // actual PC controller or keyboard input through that virtual controller.
  p.setControllerRumble(false);
  // Request a small background stream and never attach received tracks to
  // playback elements. Xbox may still encode/send Remote Play video.
  p.setVideoBitrate(512);
  p._webrtcClient.ontrack = event => { event.track.enabled = false; };
  const initial = await createOffer(p);
  const answer = await bridge.call('sdp', { sdp: initial.sdp, key: command.key });
  if (own !== generation) return;
  await p._webrtcClient.setRemoteDescription({ type: 'answer', sdp: answer.sdp });
  await Promise.race([
    new Promise(resolve => {
      if (p._webrtcClient.iceGatheringState === 'complete') return resolve();
      p._webrtcClient.addEventListener('icegatheringstatechange', () => { if (p._webrtcClient.iceGatheringState === 'complete') resolve(); });
    }), new Promise(resolve => setTimeout(resolve, 3000)),
  ]);
  const ice = await bridge.call('ice', { key: command.key, candidates: p.getIceCandidates().map(c => ({ candidate: c.candidate, sdpMLineIndex: c.sdpMLineIndex, sdpMid: c.sdpMid })) });
  if (own !== generation) return;
  for (const candidate of ice) {
    await p._webrtcClient.addIceCandidate({ candidate: candidate.candidate === 'a=end-of-candidates' ? '' : candidate.candidate, sdpMid: candidate.sdpMid, sdpMLineIndex: Number(candidate.sdpMLineIndex) });
  }
  await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => reject(new Error('Xbox did not establish the microphone connection.')), 25000);
    const check = () => {
      const state = p._webrtcClient.connectionState;
      if (state === 'connected') { clearTimeout(timeout); resolve(); }
      else if (state === 'failed' || state === 'closed' || own !== generation) { clearTimeout(timeout); reject(new Error('Xbox connection was interrupted.')); }
    };
    p._webrtcClient.addEventListener('connectionstatechange', check);
    check();
  });
  if (own !== generation) return;
  // Use the library's chat renegotiation shape, but own the track lifecycle so
  // mute/unmute never leaks an old USB-mic capture stream.
  for (const track of microphone.getAudioTracks()) p._webrtcClient.addTrack(track, microphone);
  const offer = await createOffer(p);
  const chatAnswer = await bridge.call('sdp', { sdp: offer.sdp, chat: true, key: command.key });
  if (own !== generation) return;
  await p._webrtcClient.setRemoteDescription({ type: 'answer', sdp: chatAnswer.sdp });
  connectedAt = lastAdvance = Date.now();
  report({ state: 'waiting', detail: 'Connected. Waiting for microphone packets.' });
  statsTimer = setInterval(async () => {
    if (statsBusy || own !== generation) return;
    statsBusy = true;
    try {
      const stats = await p._webrtcClient.getStats();
      let packets = 0;
      stats.forEach(value => { if (value.type === 'outbound-rtp' && (value.kind || value.mediaType) === 'audio') packets += value.packetsSent || 0; });
      if (packets > previousPackets) lastAdvance = Date.now();
      previousPackets = packets;
      const flowing = Date.now() - lastAdvance < 3000 && packets > 0;
      report({ state: muted ? 'muted' : flowing ? 'transmitting' : 'waiting', packets, detail: muted ? 'Microphone muted.' : flowing ? 'Microphone packets are being sent to Xbox.' : 'No microphone packets are leaving. Try reconnecting.' });
      if (!muted && Date.now() - connectedAt > 12000 && Date.now() - lastAdvance > 8000) throw new Error('Microphone transmission stopped. Reconnect to Xbox.');
      if (p._webrtcClient.connectionState === 'failed' || p._webrtcClient.connectionState === 'closed') throw new Error('Xbox Remote Play disconnected.');
    } catch (error) { if (own === generation) { reportError(error); await stop(); } }
    finally { statsBusy = false; }
  }, 750);
}

bridge.onCommand(async command => {
  const action = command.action === 'start' || command.action === 'stop' ? ++actionEpoch : actionEpoch;
  try {
    if (command.action === 'devices') await listMicrophones();
    else if (command.action === 'start') await start(command);
    else if (command.action === 'stop') await stop();
    else if (command.action === 'mute') {
      muted = !!command.muted;
      microphone?.getAudioTracks().forEach(track => { track.enabled = !muted; });
      report({ state: muted ? 'muted' : 'waiting', detail: muted ? 'Microphone muted.' : 'Microphone unmuted. Checking transmission.' });
    }
  } catch (error) { if (action === actionEpoch) { reportError(error); if (command.action === 'start') await stop(); } }
});
window.addEventListener('beforeunload', () => { microphone?.getTracks().forEach(track => track.stop()); });
navigator.mediaDevices.addEventListener('devicechange', async () => {
  const devices = (await navigator.mediaDevices.enumerateDevices()).filter(d => d.kind === 'audioinput');
  bridge.report({ microphones: devices.map(d => ({ id: d.deviceId, name: d.label || 'Microphone' })) });
});
bridge.ready();
