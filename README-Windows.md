XR1 Play 1.5.0 — Windows x64 preview

Extract the entire ZIP, then open XR1-Play.exe. Keep the DLLs and assets folder
beside the executable. No Python, browser or Linux compatibility layer is
needed on Windows. Do not launch the executable from inside the ZIP.

Requires Windows 10/11 x64 with Media Foundation. Windows N editions require
the Media Feature Pack. This native build has been compiled and run under
Wine for software checks; physical XR1 Lite capture on Windows is not yet
verified. See COMPATIBILITY.md before redistributing it as hardware-tested.

1. Xbox HDMI OUT -> XR1 Lite HDMI IN. Connect XR1 directly over USB 3.
2. Xbox output: 1080p, 60 Hz, SDR. HDMI audio: Stereo uncompressed.
3. In Windows Settings allow desktop apps to access the camera and microphone.
   The card's HDMI audio is a recording/input device, sometimes named
   Digital Audio Interface or USB Digital Audio.
4. Select the XR1 video source, its audio input, and speakers/headphones.
   Leave Test picture OFF for real capture. Click Start playing.

Enter starts/stops. F/F11 or double-click toggles fullscreen. Tab opens settings.
M mutes, +/- changes volume. Move the mouse for the player controls. Refresh
rescans stopped devices. Settings are saved in %APPDATA%\XR1 Play\settings.

For a device/mode report, run in a terminal:
  .\XR1-Play.exe --diagnose > xr1-diagnostics.txt 2>&1

For the optional test picture/tone:
  .\XR1-Play.exe --demo

Normal capture uses native Media Foundation; audio uses SDL/WASAPI. It requires
an advertised native 1920x1080 60/59.94 fps mode and does not interpolate 30 fps.
Volume controls, bounded audio buffering and latest-frame delivery are shared
with Linux. --capture-buffers affects Linux V4L2 only; Windows driver/source-reader
buffering is managed by Media Foundation, with the low-latency attribute enabled.

If the camera is busy, close OBS/Camera/other video apps. For silent sound check
Xbox HDMI Stereo uncompressed, the selected Windows recording endpoint, its
mute/level and Windows privacy settings. Generic device names are selectable;
the viewer does not automatically choose an unidentified microphone.

The software is unsigned: these downloads have no purchased Windows signing
certificate. SHA256SUMS accompanies this release. No blanket compatibility or
measured end-to-end latency claim is made. Report hardware tests using the
checklist in COMPATIBILITY.md.

Licenses and original icon sources are included. XR1 Play is MIT licensed;
third-party libraries/fonts/icons retain their own licenses.

This software is based in part on the work of the FreeType Team. SDL2_ttf uses
FreeType and HarfBuzz; the relevant notices are included with the application.
