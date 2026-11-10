#!/usr/bin/env python3
"""Bundle userspace libraries, leaving graphics/audio platform libraries to Bazzite."""
from pathlib import Path
import os, re, shutil, subprocess, tarfile
root=Path(__file__).resolve().parents[1]
app=root/'build/XR1Play.AppDir'
if app.exists(): shutil.rmtree(app)
for d in ('usr/bin','usr/lib','usr/share/xr1-play','usr/share/licenses'):(app/d).mkdir(parents=True,exist_ok=True)
shutil.copy2(root/'build/xr1-play',app/'usr/bin/xr1-play')
subprocess.run(['strip',str(app/'usr/bin/xr1-play')],check=True)
shutil.copy2(root/'assets/DejaVuSans.ttf',app/'usr/share/xr1-play')
shutil.copytree(root/'assets/icons',app/'usr/share/xr1-play/icons')
shutil.copy2(root/'assets/icons/LICENSE',app/'usr/share/licenses/Lucide.LICENSE')
# Keep the host's libc, graphics drivers, Wayland/libdecor plugins and audio
# backends together. Their stable ABIs are supplied by current Bazzite images.
host=re.compile(r'^(?:libc\.|libm\.|libpthread\.|libdl\.|librt\.|ld-linux|libresolv\.|libutil\.|libanl\.|libnss_|libasound\.|libpulse(?:common|-simple)?[.-]|libsystemd\.|libudev\.|libgbm\.|libdrm|libwayland|libxkbcommon\.|libdecor-)')
env=os.environ.copy();env['LD_LIBRARY_PATH']=str(root/'.deps/root/usr/lib/x86_64-linux-gnu')
listing=subprocess.check_output(['ldd',str(root/'build/xr1-play')],env=env,text=True)
if 'not found' in listing: raise RuntimeError(listing)
libs=[]
for line in listing.splitlines():
    m=re.search(r'(\S+) => (/\S+)',line)
    if not m:continue
    name,path=m.groups()
    if host.match(name):continue
    shutil.copy2(path,app/'usr/lib'/name)
    libs.append((name,path))
# Exact distribution copyright notices include licenses and upstream source URLs.
for name,path in libs:
    query=subprocess.run(['dpkg-query','-S',path],capture_output=True,text=True)
    if query.returncode:query=subprocess.run(['dpkg-query','-S',os.path.realpath(path)],capture_output=True,text=True)
    if query.returncode and path.startswith('/lib/'):
        query=subprocess.run(['dpkg-query','-S','/usr'+path],capture_output=True,text=True)
    package=query.stdout.split(':')[0] if query.returncode==0 else ''
    candidates=[Path('/usr/share/doc')/package/'copyright',root/'.deps/root/usr/share/doc'/package/'copyright']
    if 'SDL2_ttf' in name:candidates.insert(0,root/'.deps/root/usr/share/doc/libsdl2-ttf-2.0-0/copyright')
    for candidate in candidates:
        if candidate.is_file():shutil.copy2(candidate,app/'usr/share/licenses'/f'{name}.copyright');break
    else:raise RuntimeError(f'Missing license for {name}')
for path in ('/usr/share/doc/fonts-dejavu-core/copyright','/usr/share/doc/fonts-dejavu/copyright'):
    if Path(path).is_file():shutil.copy2(path,app/'usr/share/licenses/DejaVuSans.copyright');break
shutil.copy2(root/'README.md',app/'README.md')
shutil.copy2(root/'COMPATIBILITY.md',app/'COMPATIBILITY.md')
shutil.copy2(root/'README-Windows.md',app/'README-Windows.md')
shutil.copy2(root/'LICENSE',app/'usr/share/licenses/XR1-Play.LICENSE')
(app/'BUNDLED-LIBRARIES.txt').write_text('\n'.join(f'{name} ({path})' for name,path in libs)+'\n')
(app/'AppRun').write_text('''#!/bin/sh
set -eu
APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export XR1_ASSET_DIR="$APPDIR/usr/share/xr1-play"
export LD_LIBRARY_PATH="$APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PULSE_LATENCY_MSEC="${PULSE_LATENCY_MSEC:-5}"
export PIPEWIRE_LATENCY="${PIPEWIRE_LATENCY:-128/48000}"
exec "$APPDIR/usr/bin/xr1-play" "$@"
''')
(app/'AppRun').chmod(0o755)
(app/'xr1-play.desktop').write_text('''[Desktop Entry]
Type=Application
Name=XR1 Play
Comment=Low-buffering 1080p60 HDMI capture viewer with audio
Exec=xr1-play
Icon=xr1-play
Categories=AudioVideo;Video;
Terminal=false
StartupWMClass=xr1-play
''')
(app/'xr1-play.svg').write_text('''<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256"><rect width="256" height="256" rx="52" fill="#10160e"/><rect x="35" y="53" width="186" height="132" rx="14" fill="none" stroke="#9bf00b" stroke-width="12"/><path d="M107 89 L157 119 L107 149 Z" fill="#9bf00b"/><path d="M95 211 H161" stroke="#91a3b8" stroke-width="12" stroke-linecap="round"/></svg>''')
# A raster icon for file managers and AppImage thumbnailers.
import struct, zlib
pixels=bytearray()
for y in range(256):
    pixels.append(0)
    for x in range(256):
        color=(16,22,14,255)
        if (35<=x<221 and 53<=y<65) or (35<=x<221 and 173<=y<185) or (35<=x<47 and 53<=y<185) or (209<=x<221 and 53<=y<185):color=(155,240,11,255)
        if 107<=x<=157 and abs(y-119)<=(157-x)*0.6:color=(155,240,11,255)
        if 95<=x<161 and 205<=y<217:color=(145,163,184,255)
        pixels.extend(color)
def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',256,256,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(pixels))+chunk(b'IEND',b'')
(app/'xr1-play.png').write_bytes(png)
shutil.copy2(app/'xr1-play.png',app/'.DirIcon')
for path in [app,*app.rglob('*')]:
    path.chmod(0o755 if path.is_dir() or path.name=='AppRun' or path==app/'usr/bin/xr1-play' else 0o644)
dist=root/'dist';dist.mkdir(exist_ok=True)
archive=dist/'XR1-Play-1.5.0-x86_64.tar.gz'
with tarfile.open(archive,'w:gz') as tar:tar.add(app,arcname='XR1-Play')
archive.chmod(0o644)
# A single-file executable alternative for installations without FUSE.
payload=archive.read_bytes()
import hashlib
checksum=hashlib.sha256(payload).hexdigest()
header='''#!/bin/sh
# XR1 Play portable executable. No root, installer or FUSE required.
set -eu
if [ "${1:-}" = "--portable-help" ]; then
  echo 'Run this file normally. --extract DIRECTORY extracts a replaceable portable folder.'
  exit 0
fi
line=$(awk '/^__XR1_ARCHIVE_BELOW__$/ {print NR + 1; exit}' "$0")
work=$(mktemp -d "${TMPDIR:-/tmp}/xr1-play.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
# Verify the appended payload before using it.
tail -n +"$line" "$0" > "$work/payload.tar.gz"
actual=$(sha256sum "$work/payload.tar.gz")
actual=${actual%% *}
if [ "$actual" != "CHECKSUM" ]; then echo 'XR1 Play: damaged download (checksum mismatch).' >&2; exit 1; fi
if [ "${1:-}" = "--extract" ]; then
  [ "$#" = 2 ] || { echo 'Usage: --extract DIRECTORY' >&2; exit 1; }
  mkdir -p -- "$2"
  tar -xzf "$work/payload.tar.gz" -C "$2"
  echo "Extracted to $2/XR1-Play. Run its AppRun executable."
  exit 0
fi
tar -xzf "$work/payload.tar.gz" -C "$work"
"$work/XR1-Play/AppRun" "$@"
exit $?
__XR1_ARCHIVE_BELOW__
'''.replace('CHECKSUM',checksum).encode()
run=dist/'XR1-Play-1.5.0-x86_64.run';run.write_bytes(header+payload);run.chmod(0o755)
print(f'Bundled {len(libs)} libraries; portable .run and tar.gz created.')
