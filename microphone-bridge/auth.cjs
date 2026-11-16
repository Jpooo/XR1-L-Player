const UserToken = require('xal-node/dist/lib/tokens/usertoken').default;
const { delay } = require('./xhome.cjs');

// Do not save a completed sign-in after Cancel, Sign out, or a new attempt.
async function pollDeviceCode(msal, store, device, current, fetcher = fetch, wait = delay) {
  const expires = Date.now() + Math.min(900, device.expires_in || 900) * 1000;
  let interval = Math.max(1, Number(device.interval) || 5);
  while (current() && Date.now() < expires) {
    await wait(interval * 1000);
    if (!current()) return false;
    const response = await fetcher('https://login.microsoftonline.com/consumers/oauth2/v2.0/token', {
      method: 'POST', signal: AbortSignal.timeout(12000), redirect: 'error',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams({ grant_type: 'urn:ietf:params:oauth:grant-type:device_code', client_id: msal._clientId, device_code: device.device_code }).toString(),
    });
    const body = await response.json();
    if (!current()) return false;
    if (response.ok && body.access_token && body.refresh_token) {
      store.removeAll(); store.setUserToken(new UserToken(body)); store.save();
      return true;
    }
    if (body.error === 'slow_down') interval += 5;
    else if (body.error !== 'authorization_pending') throw new Error('Microsoft sign-in was declined or expired. Choose Sign in to try again.');
  }
  if (!current()) return false;
  throw new Error('Microsoft sign-in expired. Choose Sign in to try again.');
}
module.exports = { pollDeviceCode };
