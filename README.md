# XR1 Play 1.5.0

Native, live-only **1080p60 video with stereo HDMI audio** for the EVGA XR1 Lite.
Linux uses V4L2; Windows uses Media Foundation. Audio and the SDL desktop UI
are native on both systems. No recorder, server, browser or frame interpolation.
Controllers stay connected to the Xbox.

Prebuilt applications are available in the repository's **Releases** section.
Use the Linux AppImage/`.run`, or the complete Windows portable ZIP (preview).

![XR1 Play interface](docs/screenshot.png)

The headline/subtitle are removed from both versions. The shared menu is more
compact and uses neutral device/status labels. Motion, fullscreen, volume,
latest-frame handoff and the existing bounded audio queue remain available.

## Downloads and launch

**Linux x64**: make `XR1-Play-1.5.0-x86_64.AppImage` executable and open it:

```sh
chmod +x XR1-Play-1.5.0-x86_64.AppImage
./XR1-Play-1.5.0-x86_64.AppImage
```

Without FUSE, use `--appimage-extract-and-run`, or the single-file `.run`:

```sh
chmod +x XR1-Play-1.5.0-x86_64.run
./XR1-Play-1.5.0-x86_64.run
```

The `.run` verifies its payload and extracts to a temporary folder without
installing packages or requiring root. The portable `.tar.gz` can be extracted
anywhere; launch `XR1-Play/AppRun` and keep the folder together.

**Windows 10/11 x64 preview**: extract `XR1-Play-1.5.0-Windows-x64.zip` completely
and open `XR1-Play.exe`. Keep its DLLs and assets directory together. See
[README-Windows.md](README-Windows.md) for Windows setup/permissions. This build
is unsigned and has not yet been verified with a physical XR1 Lite on Windows.

**Compatibility is not universal or guaranteed.** The binaries are not a test
of every unit/firmware/host. See [COMPATIBILITY.md](COMPATIBILITY.md) for the
support boundary, tested cases and a real-hardware verification checklist.
Linux binary targets require x86_64, glibc 2.38+ and standard display/audio
libraries; current Bazzite is the primary Linux target. Older systems can build
from source. Windows N needs Microsoft's Media Feature Pack.

## Connect and play

1. Xbox HDMI OUT -> XR1 Lite HDMI IN, then XR1 directly to a USB 3 port with a
   USB 3 data cable. Close OBS/Camera/other capture applications.
2. Set Xbox to **1080p, 60 Hz, SDR**, HDMI audio **Stereo uncompressed**.
   A headset setting may mute speaker/HDMI audio; check that if sound is missing.
3. Select the XR1 video device and its **audio input**. Generic names such as
   USB Digital Audio are selectable. The app selects names containing XR1/EVGA
   when available; it does not automatically open an unidentified microphone.
4. Choose speakers/headphones, keep format Automatic, and click **Start playing**.
   Leave **Test picture off** for actual card capture.

Device selection is saved after a successful start. Linux prefers stable
`/dev/v4l/by-id` paths so renumbering need not break a saved card selection.
Windows uses native symbolic links. An explicitly requested missing ID/name
produces an error instead of silently switching devices.

Click the arrows on connection cards to choose devices. Refresh rescans while
stopped. Automatic cycles pixel formats. Back to game closes settings while
keeping capture running; Stop preview stops it. Move the mouse for the player
volume/mute/settings/fullscreen controls; they hide after three seconds.

- **Enter** starts/stops; **Tab** opens/closes settings.
- **F / F11** or double-click video toggles fullscreen.
- **M** mutes; **+ / -** changes volume. The volume slider also unmutes.
- **R** refreshes stopped devices; **Esc** leaves fullscreen, then stops/quits.
- **Motion** or `--reduce-motion` controls menu animation.

## Capture and latency behavior

The viewer requires progressive 1920x1080 at 60 or 60000/1001 fps. It tries
advertised raw formats before MJPEG/JPEG and rejects unconfirmed/30 fps modes.
Linux retries eligible formats after configuration/stream-start failures and
accepts fixed-rate evidence where optional rate ioctls are absent. Padded raw
rows/planes and a JPEG's padded 1088th-height region are handled with bounds
checks. The logical capture mode remains 1080p60.

Linux requests two driver buffers (the driver may require more) and drains
completed frames to the newest. Windows uses asynchronous native Media
Foundation samples with converters/video processing disabled and its low-latency
attribute enabled. The application handoff holds one newest pending frame,
wakes the renderer on arrival, and releases its lock before GPU upload.

The renderer explicitly requests vsync off; a compositor may still synchronize
presentation. Fullscreen requests X11 compositor bypass where supported. This
is a request, not an override of desktop policy. For comparison, try Desktop
Mode if normally running through Bazzite Gaming Mode/Gamescope. On a desktop
supporting both, compare SDL_VIDEODRIVER=x11 and SDL_VIDEODRIVER=wayland.

Audio is 48 kHz stereo PCM, requesting 128-sample callbacks (~2.67 ms), with a
20 ms **application queue cap**. The queue size shown in settings is buffered
**audio**, not Xbox-to-screen/controller latency. Hardware/audio-server buffers
are additional. The canceled queue adjustment was not applied.

```sh
./XR1-Play-1.5.0-x86_64.AppImage --audio-buffer 8
./XR1-Play-1.5.0-x86_64.AppImage --capture-buffers 3
```

Smaller audio caps can crackle; increase to 20/40 if needed. Three capture buffers
are a Linux compatibility option if a particular driver needs them. Windows
source-reader/driver buffering is managed by Media Foundation. `--vsync` is
available to reduce tearing, with possible additional presentation delay.

**No measured end-to-end latency guarantee is made.** Firmware, USB transfer,
Xbox rendering, compositor and monitor all contribute. A 60 fps frame lasts
16.67 ms. HDMI passthrough to a monitor avoids the USB preview path entirely.
The app cannot remove that hardware cost.

## Diagnostics and troubleshooting

Run `--diagnose` to list video device identifiers, USB IDs, native 1080p modes,
frame intervals and audio names. Startup logs show the negotiated mode, renderer,
backend and whether the renderer accepted the vsync request.

```sh
./XR1-Play-1.5.0-x86_64.AppImage --diagnose > xr1-diagnostics.txt 2>&1
```

Windows: `.\XR1-Play.exe --diagnose > xr1-diagnostics.txt 2>&1`.

- No card: check USB/data cable, desktop permissions and other capture apps.
- No 1080p60: check USB 3 speed, Xbox output and EVGA Lite firmware. The app
  cannot expose a mode missing from the device's firmware. Use EVGA's tools and
  instructions; it does not flash firmware. Try another advertised pixel format.
- Linux input missing: select a sound profile that includes input/duplex.
- Windows access denied: allow desktop camera and microphone access in Settings.
- No sound: select the card's USB recording endpoint, unmute it in OS settings,
  and check Xbox HDMI Stereo uncompressed/headset muting and playback output.
- Protected video: HDCP content cannot be captured; test with a normal game.
- Multiple identical devices: give audio endpoints distinct OS names if they
  appear identical. Automatic multi-card audio pairing is not verified.

Linux settings: `${XDG_CONFIG_HOME:-$HOME/.config}/xr1-play/settings`.
Windows settings: `%APPDATA%\XR1 Play\settings`. Delete the file to reset choices.

The settings footer reports capture/display fps, skipped/invalid frames and the
application's audio queue. Logs also report audio overflow/underflow samples,
ready-to-upload and presentation-call durations. Those exclude device firmware,
display scanout and delays after presentation returns.

## Testing and source builds

See [tests/VALIDATION.md](tests/VALIDATION.md) and [COMPATIBILITY.md](COMPATIBILITY.md).
Software/mocked/emulated tests do not establish physical capture compatibility.
The optional `--demo` / Test picture shows generated test bars and a stereo tone;
normal capture always uses the selected physical card.

Install C++17, CMake, SDL2, SDL2_ttf and libjpeg-turbo development dependencies:

```sh
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake --parallel
ctest --test-dir build-cmake --output-on-failure
```

On Windows, use MSYS2 MINGW64 packages `mingw-w64-x86_64-toolchain`,
`mingw-w64-x86_64-cmake`, `mingw-w64-x86_64-SDL2`,
`mingw-w64-x86_64-SDL2_ttf`, and `mingw-w64-x86_64-libjpeg-turbo`.
The CMake build copies assets next to the executable. DLLs must remain available
on PATH or be collected into a portable folder. `tools/package-windows.py`
checks non-system PE imports recursively for the cross-build release environment.
For a native development build, keep the DLLs available on PATH or copy them
beside the executable, and retain the appropriate third-party notices.

The original Linux Makefile supports `make DEPS=/usr` and `make DEPS=/usr test`.
On immutable Bazzite do development in toolbox/distrobox rather than layering
build packages on the host. Prebuilt apps do not need those development tools.
`tools/build-windows.sh` reproduces the cross-build using the pinned SDL SDKs
and local MinGW toolchain. `tools/package.py` and `tools/build-appimage.sh`
produce the Linux portable artifacts on the Debian build machine.

XR1 Play is MIT licensed. Libraries, fonts and Lucide icons retain their own
licenses in the portable folders. The icon source SVGs and pinned version are
included under assets/icons. Applications do not require SVG or Python at
runtime. Distribution/library provenance and notices accompany each platform.

This software is based in part on the work of the FreeType Team. SDL2_ttf uses
FreeType and HarfBuzz; the relevant notices are included with the application.
