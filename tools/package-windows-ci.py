#!/usr/bin/env python3
"""Bundle the native Windows CI executable, non-system DLLs and required assets.

This is a diagnostic TEST package. It is never published as a release.
"""
from pathlib import Path
import hashlib
import os
import re
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
executable = root / "build-windows-tests" / "XR1-L-Player.exe"
if not executable.is_file():
    raise SystemExit(f"Windows executable not found: {executable}")

mingw_prefix = Path(os.environ["MINGW_PREFIX"])
search_paths = [
    executable.parent,
    root / ".deps/windows-ci/SDL2-2.32.4/x86_64-w64-mingw32/bin",
    root / ".deps/windows-ci/SDL2_ttf-2.24.0/x86_64-w64-mingw32/bin",
    mingw_prefix / "bin",
]
dlls = {
    path.name.lower(): path
    for folder in search_paths
    if folder.is_dir()
    for path in folder.iterdir()
    if path.is_file() and path.suffix.lower() == ".dll"
}
system = set("""
kernel32.dll kernelbase.dll ntdll.dll msvcrt.dll ucrtbase.dll user32.dll
gdi32.dll gdi32full.dll advapi32.dll ole32.dll oleaut32.dll combase.dll
setupapi.dll shell32.dll shlwapi.dll shcore.dll propsys.dll mf.dll mfplat.dll
mfreadwrite.dll mfuuid.dll rpcrt4.dll imm32.dll version.dll winmm.dll
ws2_32.dll bcrypt.dll crypt32.dll secur32.dll usp10.dll d3d11.dll d3d9.dll
dxgi.dll dwrite.dll d2d1.dll winhttp.dll wininet.dll iphlpapi.dll
winspool.drv avrt.dll cfgmgr32.dll powrprof.dll dwmapi.dll userenv.dll
mpr.dll msimg32.dll shfolder.dll normaliz.dll oleacc.dll
""".split())
destination = root / "dist" / "XR1-L-Player-Windows-Diagnostics"
if destination.exists():
    shutil.rmtree(destination)
destination.mkdir(parents=True)
shutil.copy2(executable, destination / executable.name)

queue = [executable]
copied = set()
while queue:
    binary = queue.pop()
    details = subprocess.check_output(["objdump", "-p", str(binary)], text=True)
    for imported in re.findall(r"DLL Name:\s+(\S+)", details):
        key = imported.lower()
        if key in system or key.startswith(("api-ms-", "ext-ms-")) or key in copied:
            continue
        dependency = dlls.get(key)
        if dependency is None:
            raise RuntimeError(f"Cannot package, missing DLL dependency: {imported} (from {binary.name})")
        shutil.copy2(dependency, destination / imported)
        copied.add(key)
        queue.append(dependency)

shutil.copytree(root / "assets", destination / "assets")
licenses = destination / "licenses"
licenses.mkdir()
for rel, name in [
    ("LICENSE", "XR1-L-Player.txt"),
    ("assets/DejaVuSans.LICENSE", "DejaVuSans.txt"),
    ("assets/fonts/ChakraPetch-OFL.txt", "ChakraPetch-OFL.txt"),
    ("assets/icons/LICENSE", "Lucide.txt"),
]:
    source = root / rel
    if source.is_file():
        shutil.copy2(source, licenses / name)
for rel, name in [
    (".deps/windows-ci/SDL2-2.32.4/LICENSE.txt", "SDL2.txt"),
    (".deps/windows-ci/SDL2_ttf-2.24.0/LICENSE.txt", "SDL2_ttf.txt"),
]:
    source = root / rel
    if source.is_file():
        shutil.copy2(source, licenses / name)
shutil.copy2(root / "README-Windows.md", destination / "README-Windows.txt")
(destination / "TEST-BUILD.txt").write_text(
    "XR1-L Player - Windows diagnostics TEST BUILD\n"
    "Do not redistribute as the public Beta V2 release.\n"
    "Run XR1-L-Player.exe, play normally, then stop playback.\n"
    "The latest report is saved in:\n"
    "%APPDATA%\\XR1-L Player\\last-session-diagnostics.txt\n"
    "The report does not measure end-to-end A/V sync.\n"
)
archive = root / "dist" / "XR1-L-Player-Windows-Diagnostics-x64.zip"
with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as output:
    for path in destination.rglob("*"):
        if path.is_file():
            output.write(path, Path("XR1-L-Player") / path.relative_to(destination))
with zipfile.ZipFile(archive) as check:
    if check.testzip() is not None:
        raise RuntimeError("Created ZIP failed CRC verification")
print(f"Created {archive.name}: {archive.stat().st_size / 1024 / 1024:.2f} MiB, {len(copied)} runtime DLLs")
print("SHA256", hashlib.sha256(archive.read_bytes()).hexdigest())
