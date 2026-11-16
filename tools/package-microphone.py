#!/usr/bin/env python3
"""Assemble the local-only microphone experiment, retaining verified V2 packages."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
bridge = root / 'microphone-bridge'
out = root / 'dist'
out.mkdir(exist_ok=True)

def bundle():
    subprocess.run(['npm', '--cache', '/workspace/.cache/npm', 'run', 'bundle'], cwd=bridge, check=True)
    subprocess.run([str(bridge/'node_modules/.bin/esbuild'), 'main.cjs', '--bundle', '--platform=node', '--external:electron', '--outfile=main.bundle.cjs'], cwd=bridge, check=True)

def app_files(dest):
    dest.mkdir(parents=True, exist_ok=True)
    for name in ['main.bundle.cjs', 'preload.cjs', 'renderer.html', 'renderer.bundle.js']:
        shutil.copy2(bridge/name, dest/('main.cjs' if name=='main.bundle.cjs' else name))
    (dest/'package.json').write_text(json.dumps({'name':'xr1-microphone-bridge','version':'0.1.0','main':'main.cjs'}))
    licenses=dest/'licenses';licenses.mkdir(exist_ok=True)
    for name in ['xal-node', 'xbox-webapi', 'ip-address', 'events', 'jose', 'uuid', 'uuid4']:
        source=bridge/'node_modules'/name
        notice=next(p for p in [source/'LICENSE',source/'LICENSE.md'] if p.is_file())
        shutil.copy2(notice,licenses/(name+'.txt'))
    shutil.copy2(bridge/'Greenlight-LICENSE.txt', licenses/'Greenlight.txt')
    (licenses/'xbox-xcloud-player.txt').write_text('xbox-xcloud-player 0.2.11\nAuthor: Jim Kroon (UnknownSKL). License: MIT.\nhttps://github.com/unknownskl/xbox-xcloud-player\n\n'+(bridge/'Greenlight-LICENSE.txt').read_text())
    shutil.copy2(root/'MICROPHONE-TEST.md', dest/'MICROPHONE-TEST.md')
    for item in [dest, *dest.rglob('*')]:
        item.chmod(0o755 if item.is_dir() else 0o644)
    for p in [dest, *dest.rglob('*')]:
        p.chmod(0o755 if p.is_dir() else 0o644)

def copy_link(src, dest):
    os.link(src, dest)
    return dest

def linux():
    subprocess.run(['python3','tools/package.py'], cwd=root, check=True)
    app=root/'build/XR1LPlayer.AppDir'
    runtime=app/'usr/bin/microphone-bridge'
    shutil.copytree(bridge/'node_modules/electron/dist', runtime, copy_function=copy_link)
    (runtime/'electron').rename(runtime/'xr1-microphone')
    for item in [runtime, *runtime.rglob('*')]:
        item.chmod(0o755 if item.is_dir() or item.name in ['xr1-microphone','chrome-sandbox','chrome_crashpad_handler'] else 0o644)
    app_files(runtime/'resources/app')
    for p in [runtime, *runtime.rglob('*')]:
        p.chmod(0o755 if p.is_dir() or p.name in ['xr1-microphone','chrome-sandbox','chrome_crashpad_handler'] else 0o644)
    shutil.copy2(root/'MICROPHONE-TEST.md', app/'MICROPHONE-TEST.md')
    target=out/'XR1-L-Player-V2-Microphone-Test-1.5.44-Linux-x86_64.AppImage'
    target.unlink(missing_ok=True)
    env=os.environ.copy();env['ARCH']='x86_64'
    subprocess.run([str(root/'.deps/appimagetool.AppImage'),'--appimage-extract-and-run',str(app),str(target)], env=env, check=True)
    target.chmod(0o755)
    archive=target.with_suffix('.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        z.write(target,target.name)
        z.write(root/'MICROPHONE-TEST.md','START-HERE.md')
    archive.chmod(0o644)
    print(archive)

def windows():
    subprocess.run(['python3','tools/package-windows.py'],cwd=root,check=True)
    app=root/'build/XR1-L-Player-Windows'
    runtime=app/'microphone-bridge';runtime.mkdir(exist_ok=True)
    artifacts=list(Path('/workspace/.cache/electron').rglob('electron-v45.3.2-win32-x64.zip'))
    assert len(artifacts)==1, 'Download the verified Windows Electron distribution first'
    with zipfile.ZipFile(artifacts[0]) as archive:archive.extractall(runtime)
    (runtime/'electron.exe').rename(runtime/'XR1-Microphone.exe')
    app_files(runtime/'resources/app')
    shutil.copy2(root/'MICROPHONE-TEST.md', app/'MICROPHONE-TEST.md')
    archive=out/'XR1-L-Player-V2-Microphone-Test-1.5.44-Windows-x64.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for p in sorted(app.rglob('*')):
            if p.is_file():z.write(p,Path('XR1-L-Player-Microphone-Test')/p.relative_to(app))
    archive.chmod(0o644)
    print(archive)

if __name__=='__main__':
    import sys
    bundle()
    if 'linux' in sys.argv:linux()
    if 'windows' in sys.argv:windows()
    paths=sorted(out.glob('XR1-L-Player-V2-Microphone-Test-1.5.44-*'))
    (out/'MICROPHONE-TEST-SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in paths if p.is_file()))
