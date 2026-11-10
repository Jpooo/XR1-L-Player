#!/usr/bin/env python3
"""Bundle userspace libraries, leaving graphics/audio platform libraries to Bazzite."""
from pathlib import Path
import os, re, shutil, subprocess, tarfile
root=Path(__file__).resolve().parents[1]
app=root/'build/XR1Play.AppDir'
if app.exists(): shutil.rmtree(app)
for d in ('usr/bin','usr/lib','usr/share/xr1-l-player','usr/share/licenses'):(app/d).mkdir(parents=True,exist_ok=True)
shutil.copy2(root/'build/xr1-play',app/'usr/bin/xr1-l-player')
subprocess.run(['strip',str(app/'usr/bin/xr1-l-player')],check=True)
shutil.copy2(root/'assets/DejaVuSans.ttf',app/'usr/share/xr1-l-player')
shutil.copytree(root/'assets/icons',app/'usr/share/xr1-l-player/icons')
shutil.copytree(root/'assets/branding',app/'usr/share/xr1-l-player/branding')
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
shutil.copy2(root/'README-Linux.md',app/'README.md')
shutil.copy2(root/'COMPATIBILITY.md',app/'COMPATIBILITY.md')
shutil.copy2(root/'LICENSE',app/'usr/share/licenses/XR1-L-Player.LICENSE')
(app/'BUNDLED-LIBRARIES.txt').write_text('\n'.join(f'{name} ({path})' for name,path in libs)+'\n')
(app/'AppRun').write_text('''#!/bin/sh
set -eu
APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export XR1_ASSET_DIR="$APPDIR/usr/share/xr1-l-player"
export LD_LIBRARY_PATH="$APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PULSE_LATENCY_MSEC="${PULSE_LATENCY_MSEC:-5}"
export PIPEWIRE_LATENCY="${PIPEWIRE_LATENCY:-128/48000}"
exec "$APPDIR/usr/bin/xr1-l-player" "$@"
''')
(app/'AppRun').chmod(0o755)
(app/'xr1-l-player.desktop').write_text('''[Desktop Entry]
Type=Application
Name=XR1-L Player
Comment=Low-buffering 1080p60 HDMI capture viewer with audio
Exec=xr1-l-player
Icon=xr1-l-player
Categories=AudioVideo;Video;
Terminal=false
StartupWMClass=xr1-l-player
''')
shutil.copy2(root/'assets/branding/xr1-play-xbox.svg',app/'xr1-l-player.svg')
shutil.copy2(root/'assets/branding/xr1-play-xbox.png',app/'xr1-l-player.png')
shutil.copy2(app/'xr1-l-player.png',app/'.DirIcon')
for path in [app,*app.rglob('*')]:
    path.chmod(0o755 if path.is_dir() or path.name=='AppRun' or path==app/'usr/bin/xr1-l-player' else 0o644)
dist=root/'dist';dist.mkdir(exist_ok=True)
archive=dist/'XR1-L-Player-1.5.7-Linux-Test-x86_64.tar.gz'
with tarfile.open(archive,'w:gz') as tar:tar.add(app,arcname='XR1-L-Player')
archive.chmod(0o644)
print(f'Bundled {len(libs)} libraries; portable tar.gz created.')
