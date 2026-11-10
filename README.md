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
