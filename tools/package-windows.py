#!/usr/bin/env python3
"""Collect non-system PE imports recursively and produce a portable Windows ZIP."""
from pathlib import Path
import os,re,shutil,subprocess,zipfile
root=Path(__file__).resolve().parents[1]
exe=Path(os.environ.get('XR1_WINDOWS_EXE',root/'build/windows/XR1-L-Player.exe')).resolve()
objdump=os.environ.get('CROSS_OBJDUMP',str(root/'.deps/toolchain/usr/bin/x86_64-w64-mingw32-objdump'))
roots=[exe.parent,root/'.deps/win-sdk/SDL2-2.32.4/x86_64-w64-mingw32/bin',root/'.deps/win-sdk/SDL2_ttf-2.24.0/x86_64-w64-mingw32/bin',root/'.deps/win-sdk/mingw64/bin',root/'.deps/toolchain/usr/x86_64-w64-mingw32/lib',root/'.deps/toolchain/usr/lib/gcc/x86_64-w64-mingw32/14-posix']
if os.environ.get('MINGW_PREFIX'):roots.append(Path(os.environ['MINGW_PREFIX'])/'bin')
lookup={p.name.lower():p for folder in roots if folder.is_dir() for p in folder.glob('*') if p.is_file() and p.suffix.lower()=='.dll'}
system=set('kernel32.dll ntdll.dll msvcrt.dll ucrtbase.dll user32.dll gdi32.dll gdi32full.dll advapi32.dll ole32.dll oleaut32.dll combase.dll setupapi.dll shell32.dll shlwapi.dll shcore.dll propsys.dll mf.dll mfplat.dll mfreadwrite.dll rpcrt4.dll imm32.dll version.dll winmm.dll ws2_32.dll bcrypt.dll crypt32.dll secur32.dll usp10.dll d3d11.dll d3d9.dll dxgi.dll dwrite.dll d2d1.dll winhttp.dll wininet.dll iphlpapi.dll winspool.drv avrt.dll'.split())
folder=root/'build/XR1-L-Player-Windows'
if folder.exists():shutil.rmtree(folder)
folder.mkdir();shutil.copy2(exe,folder/'XR1-L-Player.exe')
queue=[exe];copied=set()
while queue:
    path=queue.pop()
    headers=subprocess.check_output([objdump,'-p',str(path)],text=True)
    for name in re.findall(r'DLL Name:\s+(\S+)',headers):
        key=name.lower()
        if key in system or key.startswith(('api-ms-','ext-ms-')):continue
        if key in copied:continue
        if key not in lookup:raise RuntimeError('Missing dependency: '+name)
        dependency=lookup[key];shutil.copy2(dependency,folder/name);copied.add(key);queue.append(dependency)
shutil.copytree(root/'assets',folder/'assets')
(folder/'licenses').mkdir()
shutil.copy2(root/'LICENSE',folder/'licenses/XR1-L-Player.txt')
shutil.copy2(root/'assets/DejaVuSans.LICENSE',folder/'licenses/DejaVuSans.txt')
shutil.copy2(root/'assets/icons/LICENSE',folder/'licenses/Lucide.txt')
for label,path in {'SDL2':root/'.deps/win-sdk/SDL2-2.32.4/LICENSE.txt','SDL2_ttf':root/'.deps/win-sdk/SDL2_ttf-2.24.0/LICENSE.txt','GCC-runtime':root/'.deps/toolchain/usr/share/doc/gcc-mingw-w64-base/copyright','MinGW-runtime':root/'.deps/toolchain/usr/share/doc/mingw-w64-common/copyright'}.items():
    if path.is_file():shutil.copy2(path,folder/'licenses'/f'{label}.txt')
for path in (root/'.deps/win-sdk/mingw64/share/licenses').glob('*jpeg*'):
    shutil.copytree(path,folder/'licenses'/path.name,dirs_exist_ok=True)
shutil.copy2(root/'README-Windows.md',folder/'README.txt')
shutil.copy2(root/'COMPATIBILITY.md',folder/'COMPATIBILITY.md')
shutil.copy2('/usr/share/doc/libfreetype6/copyright',folder/'licenses/FreeType-notice.txt')
shutil.copy2('/usr/share/doc/libharfbuzz0b/copyright',folder/'licenses/HarfBuzz-notice.txt')
(folder/'dependencies.txt').write_text('Native Windows 10/11 x64; Media Foundation video and SDL WASAPI audio.\nBundled DLLs:\n'+'\n'.join(sorted(copied))+'\nHardware compatibility is not universally verified. See COMPATIBILITY.md.\n')
archive=root/'dist/XR1-L-Player-1.5.18-Windows-x64.zip';archive.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for path in folder.rglob('*'):
        if path.is_file():z.write(path,Path('XR1-L-Player')/path.relative_to(folder))
archive.chmod(0o644);print(f'{archive.name}: {archive.stat().st_size/1024/1024:.2f} MiB; {len(copied)} DLLs with imports resolved.')
