const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { encodeState, readCommands, cleanError, writeState } = require('../protocol.cjs');
const { XHome } = require('../xhome.cjs');
const { pollDeviceCode } = require('../auth.cjs');
const response = (body, status = 200) => new Response(body === null ? null : JSON.stringify(body), { status });

test('IPC preserves unicode labels without allowing new fields or partial writes', () => {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'xr1-protocol-test-'));
  try {
    writeState(dir, { state: 'idle', 'mic.0': 'USB 🎤\nstate=transmitting' });
    const raw = fs.readFileSync(path.join(dir, 'status'), 'utf8');
    assert.equal(raw.split('\n').filter(v => v.startsWith('state=')).length, 1);
    assert.match(encodeState({ code: '' }), /code=\n/);
    fs.writeFileSync(path.join(dir, 'cmd-00000002'), 'mute\t1\n');
    fs.writeFileSync(path.join(dir, 'cmd-00000001'), 'stop\n');
    fs.writeFileSync(path.join(dir, 'cmd-00000003.tmp'), 'quit\n');
    assert.deepEqual(readCommands(dir), [['stop'], ['mute', '1']]);
    assert.equal(readCommands(dir).length, 0);
  } finally { fs.rmSync(dir, { recursive: true }); }
});
test('errors do not expose bearer credentials or multiline payloads', () => {
  assert.equal(cleanError(new Error('HTTP Bearer secret\nRetry')), 'HTTP [redacted] Retry');
  assert.doesNotMatch(cleanError({ token: 'private' }), /private/);
});
test('Remote Play session uses authenticated home endpoint and chat renegotiation', async () => {
  const calls = [];
  const api = new XHome('https://uks.gssv-play-prod.xboxlive.com/', 'private', async (url, args) => {
    calls.push({ url, args });
    if (url.endsWith('/play')) return response({ sessionPath: '/v5/sessions/home/abc-123' });
    if (args.method === 'GET') return response({ exchangeResponse: JSON.stringify({ sdp: 'answer' }) });
    return response({});
  });
  await api.start('MY-XBOX');
  assert.equal(api.id, 'abc-123');
  assert.equal(JSON.parse(calls[0].args.body).serverId, 'MY-XBOX');
  assert.equal(calls[0].args.headers.Authorization, 'Bearer private');
  await api.sdp('offer');
  assert.equal(JSON.parse(calls[1].args.body).configuration.chatConfiguration.sampleFrequencyHz, 24000);
  await api.sdp('mic-offer', true);
  assert.equal(JSON.parse(calls[3].args.body).configuration.isMediaStreamsChatRenegotiation, true);
  await api.stop();
  assert.equal(calls.at(-1).args.method, 'DELETE');
  await assert.rejects(api.request('/v5/sessions/home/play'), /cancelled/);
});
test('rejects unexpected streaming hosts, session paths and HTTP failures', async () => {
  assert.throws(() => new XHome('https://example.com', 'secret'), /host/);
  assert.throws(() => new XHome('http://x.xboxlive.com', 'secret'), /host/);
  const bad = new XHome('x.xboxlive.com', 'secret', async () => response({ sessionPath: '/v5/sessions/home/../../bad?' }));
  await assert.rejects(bad.start('x'), /invalid session/);
  const denied = new XHome('x.xboxlive.com', 'secret', async () => response({}, 401));
  await assert.rejects(denied.start('x'), /HTTP 401/);
});
test('sign-in polling honours server backoff and saves only a current completed login', async () => {
  const waits = [], changes = [];
  const replies = [response({ error: 'authorization_pending' }, 400), response({ error: 'slow_down' }, 400), response({ access_token: 'test-access', refresh_token: 'test-refresh', expires_in: 3600 })];
  const store = { removeAll: () => changes.push('clear'), setUserToken: () => changes.push('token'), save: () => changes.push('save') };
  await pollDeviceCode({ _clientId: 'test' }, store, { device_code: 'test', interval: 5 }, () => true, async () => replies.shift(), async ms => waits.push(ms));
  assert.deepEqual(waits, [5000, 5000, 10000]);
  assert.deepEqual(changes, ['clear', 'token', 'save']);
});
test('cancelled sign-in never writes returned tokens', async () => {
  let current = true, writes = 0;
  const store = { removeAll() { writes++; }, setUserToken() { writes++; }, save() { writes++; } };
  const result = await pollDeviceCode({ _clientId: 'test' }, store, { device_code: 'test' }, () => current,
    async () => { current = false; return response({ access_token: 'secret', refresh_token: 'secret' }); }, async () => {});
  assert.equal(result, false); assert.equal(writes, 0);
});
