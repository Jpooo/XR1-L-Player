# XR1 Play 1.5.0 compatibility

XR1 Play targets the **EVGA XR1 Lite**, using standard USB video and audio
interfaces. It does not require a specific firmware string, serial number,
USB port number, or a device-name whitelist. The known Lite USB ID
`3842:310a` improves automatic selection; unrecognized IDs remain selectable.

**Compatibility with every physical unit, firmware revision and host is not
verified or guaranteed.** There is no card attached to this build environment.
A unit that does not expose 1080p60 cannot be made to supply real 1080p60 by
software. The viewer refuses an unconfirmed/30 fps mode rather than inventing
or duplicating frames to label it 60 fps.

## Platforms

| Build | Intended platform | Verified here | Physical capture status |
|---|---|---|---|
| Linux x64 | Current Bazzite; other x64 desktops with glibc 2.38+ and standard Wayland/X11/audio libraries | Compiled; mock V4L2 and address/undefined-behavior sanitizer tests; GPU/software rendering; stereo callback and controls tests | The user reports this Linux build works perfectly with their XR1 Lite on Bazzite. Exact firmware/USB-controller details are not recorded; this verifies one reported setup, not every unit. |
| Windows x64 preview | Windows 10/11 desktop with Media Foundation and normal UVC/UAC drivers | Cross-compiled native Windows PE executable; started under Wine; shared-codec tests and video/audio demo | The user tested 1.5.0 on Windows 11: capture/audio work, but video cadence fluctuates and sound can crackle. Windows 1.5.1 needs a fresh hardware test. |

Wine is a test aid here, not the Windows capture backend. The Windows program
uses native Media Foundation capture and SDL's native Windows audio support.
Windows N editions need Microsoft's Media Feature Pack. ARM/32-bit binaries,
macOS, Windows 7, and every Linux distribution are not covered by these builds.
Building the source locally may support additional Linux systems.

## Capture coverage

- 1920x1080 progressive at 60 or 60000/1001 fps.
- YUYV/YUY2, NV12, UYVY, I420/YU12, YV12, and MJPEG/JPEG where exposed by
  the OS. Advertised formats are inspected; no driver is forced to invent one.
- Linux retries other eligible formats when configuration/stream startup fails,
  and increases the buffer request if the driver rejects a smaller count.
- Linux accepts confirmed S_PARM/G_PARM rates or a single, advertised fixed
  60 fps interval when optional rate controls are absent. Conflicting 30 fps
  reports and ambiguous unconfirmed rates are rejected.
- Raw row/plane padding, zero default stride, and a missing final padding tail
  are handled with explicit bounds checks. 1088-line JPEG padding is cropped
  only after negotiating a logical 1080-line mode.
- Linux prefers stable `/dev/v4l/by-id` paths where available. Windows selects
  native camera symbolic links and inspects video streams for eligible modes.
- Missing explicit device names/IDs fail clearly; they do not silently open
  another camera or microphone. Generic USB audio labels remain selectable.
- Disconnects/invalid streams produce errors. Unrelated audio-device removal
  no longer stops the active capture.

This does not validate every USB controller, firmware, video timing, HDR mode,
protected-content behavior, or simultaneous multi-card audio pairing.

## Hardware checklist before sharing a compatibility report

1. Connect directly using a USB 3 data cable/port. Set Xbox to 1080p, 60 Hz,
   SDR and HDMI **Stereo uncompressed**. Close other capture applications.
2. Run `--diagnose` and retain the video name, USB ID, native 1080p rates,
   OS/audio backend and selected input/output names.
3. Start real capture with Test picture off. Verify moving video and HDMI sound,
   around 60 received/displayed fps for at least five minutes, and no persistent
   invalid frames. Compare fullscreen/windowed behavior.
4. Stop/restart, unplug/reconnect, and refresh devices. Check mute, volume,
   fullscreen, audio routing and startup after a reboot.
5. Record the card firmware if known, OS version, USB speed, selected format,
   and whether the default two-buffer request or `--capture-buffers 3` worked.

Do not call a unit/firmware verified from a demo, Wine run, compile, or mock test.
`Test picture` is a separate generated test source; normal capture always uses
actual frames from the selected card.

## Firmware and reference material

- EVGA Lite guide/specifications: https://www.evga.com/Support/manuals/files/141-U1-CB20-LR.pdf
- EVGA downloads (choose the **XR1 Lite** firmware, not XR1/Pro): https://www.evga.com/support/download/
- Linux setup report: https://interfacinglinux.com/2024/01/08/1080p60-capture-with-evga-xr1-lite-on-linux/
- A community report of older firmware exposing fewer modes: https://github.com/teklynk/raspi-streamer

Firmware changes are not performed by this application. Use EVGA's tools and
instructions if an older unit does not advertise the required mode. USB 2,
a charge-only cable, HDCP, or an unsuitable HDMI signal cannot be corrected
by labeling a lower-resolution/rate stream as 1080p60.
