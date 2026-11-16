// Xbox Remote Play exchange shapes follow Greenlight (MIT), UnknownSKL.
const { Address6 } = require('ip-address');
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

class XHome {
  constructor(host, token, fetcher = fetch) {
    const url = new URL(host.startsWith('https://') ? host : `https://${host}`);
    if (!url.hostname.endsWith('.xboxlive.com') || url.protocol !== 'https:' || url.port) throw new Error('Invalid Xbox streaming host.');
    this.base = url.origin;
    this.token = token;
    this.fetcher = fetcher;
    this.cancelled = false;
    this.controllers = new Set();
  }
  async request(endpoint, method = 'GET', body, headers = {}, poll = false) {
    if (!/^\/v[56]\//.test(endpoint)) throw new Error('Invalid Xbox endpoint.');
    const deadline = Date.now() + 45000;
    do {
      if (this.cancelled && method !== 'DELETE') throw new Error('Connection cancelled.');
      const controller = new AbortController();
      this.controllers.add(controller);
      const timer = setTimeout(() => controller.abort(), 12000);
      let response;
      try {
        response = await this.fetcher(this.base + endpoint, {
          method, signal: controller.signal, redirect: 'error',
          headers: { Authorization: `Bearer ${this.token}`, 'Content-Type': 'application/json', ...headers },
          body: body === undefined ? undefined : JSON.stringify(body),
        });
        if (!response.ok) throw new Error(`Xbox connection returned HTTP ${response.status}.`);
        if (response.status !== 204) {
          const text = await response.text();
          return text ? JSON.parse(text) : {};
        }
      } finally {
        clearTimeout(timer);
        this.controllers.delete(controller);
      }
      if (!poll) return {};
      await delay(750);
    } while (Date.now() < deadline);
    throw new Error('Xbox signalling timed out.');
  }
  async start(serverId) {
    const deviceInfo = {
      appInfo: { env: { clientAppId: 'Microsoft.GamingApp', clientAppType: 'native', clientAppVersion: '2203.1001.4.0', clientSdkVersion: '8.5.2', httpEnvironment: 'prod', sdkInstallId: '' } },
      dev: { hw: { make: 'Microsoft', model: 'Surface Pro', sdktype: 'native' }, os: { name: 'Windows 11', ver: '22631.2715', platform: 'desktop' }, displayInfo: { dimensions: { widthInPixels: 1280, heightInPixels: 720 }, pixelDensity: { dpiX: 1, dpiY: 1 } } },
    };
    const result = await this.request('/v5/sessions/home/play', 'POST', {
      titleId: '', systemUpdateGroup: '', clientSessionId: '', serverId, fallbackRegionNames: [],
      settings: { nanoVersion: 'V3;WebrtcTransport.dll', enableTextToSpeech: false, highContrast: 0, locale: 'en-US', useIceConnection: false, timezoneOffsetMinutes: -new Date().getTimezoneOffset(), sdkType: 'web', osName: 'windows' },
    }, { 'X-MS-Device-Info': JSON.stringify(deviceInfo) });
    const id = result.sessionPath?.split('/').filter(Boolean).pop();
    if (!id || !/^[a-zA-Z0-9_-]{1,128}$/.test(id)) throw new Error('Xbox returned an invalid session.');
    this.id = id;
    return id;
  }
  endpoint(suffix = '') {
    if (!this.id) throw new Error('No active Xbox session.');
    return `/v5/sessions/home/${this.id}${suffix}`;
  }
  state() { return this.request(this.endpoint('/state')); }
  connect(token) { return this.request(this.endpoint('/connect'), 'POST', { userToken: token }); }
  keepalive() { return this.request(this.endpoint('/keepalive'), 'POST', {}); }
  async exchange(kind, body) {
    await this.request(this.endpoint('/' + kind), 'POST', body);
    const result = await this.request(this.endpoint('/' + kind), 'GET', undefined, {}, true);
    if (typeof result.exchangeResponse !== 'string') throw new Error('Invalid Xbox signalling response.');
    return JSON.parse(result.exchangeResponse);
  }
  sdp(sdp, chat = false) {
    const configuration = chat ? { isMediaStreamsChatRenegotiation: true } : {
      chatConfiguration: { bytesPerSample: 2, expectedClipDurationMs: 20, format: { codec: 'opus', container: 'webm' }, numChannels: 1, sampleFrequencyHz: 24000 },
      chat: { minVersion: 1, maxVersion: 1 }, control: { minVersion: 1, maxVersion: 3 }, input: { minVersion: 1, maxVersion: 8 }, message: { minVersion: 1, maxVersion: 1 },
    };
    return this.exchange('sdp', { messageType: 'offer', sdp, configuration });
  }
  async ice(candidates) {
    const response = await this.exchange('ice', { messageType: 'iceCandidate', candidate: candidates });
    const computed = [];
    for (const candidate of response) {
      const address = candidate.candidate?.split(' ')[4];
      if (address?.startsWith('2001:')) {
        try {
          const teredo = new Address6(address).inspectTeredo();
          for (const port of new Set([9002, Number(teredo.udpPort)])) computed.push({ candidate: `a=candidate:10 1 UDP 1 ${teredo.client4} ${port} typ host`, sdpMLineIndex: 0, sdpMid: '0' });
        } catch { /* retain original candidate */ }
      }
      computed.push(candidate);
    }
    return computed;
  }
  async stop() {
    this.cancelled = true;
    for (const controller of this.controllers) controller.abort();
    if (this.id) {
      await this.request(this.endpoint(), 'DELETE').catch(() => {});
      this.id = undefined;
    }
  }
}
module.exports = { XHome, delay };
