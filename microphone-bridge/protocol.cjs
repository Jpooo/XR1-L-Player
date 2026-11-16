const fs = require('node:fs');
const path = require('node:path');

function encodeState(state) {
  const fields = { ...state, updated: String(Date.now()) };
  return Object.entries(fields).map(([k, v]) => `${k}=${Buffer.from(String(v)).toString('hex')}`).join('\n') + '\n';
}
function writeState(dir, state) {
  const dest = path.join(dir, 'status');
  fs.writeFileSync(dest + '.tmp', encodeState(state), { mode: 0o600 });
  fs.renameSync(dest + '.tmp', dest);
}
function readCommands(dir) {
  return fs.readdirSync(dir).filter(n => /^cmd-\d+$/.test(n)).sort().map(n => {
    const p = path.join(dir, n);
    const data = fs.readFileSync(p, 'utf8');
    fs.unlinkSync(p);
    return data.trim().split('\t');
  });
}
function cleanError(error) {
  const text = typeof error?.message === 'string' ? error.message : 'The Xbox connection failed. Try signing in again.';
  return text.replace(/Bearer\s+\S+|XBL3\.0\s+\S+/gi, '[redacted]').replace(/[\r\n]/g, ' ').slice(0, 240);
}
module.exports = { encodeState, writeState, readCommands, cleanError };
