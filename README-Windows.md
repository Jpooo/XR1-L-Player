XR1 Play 1.5.5 (local test build) — Windows x64 preview

Extract the entire ZIP, then open XR1-Play.exe. Keep the DLLs and assets folder
beside the executable. No Python, browser or Linux compatibility layer is
needed on Windows. Do not launch the executable from inside the ZIP.

Requires Windows 10/11 x64 with Media Foundation. Windows N editions require
the Media Feature Pack. Version 1.5.1 receives about 60 fps on the user's Windows machine but displays
about 58. Version 1.5.2 tests alternating reusable GPU upload destinations and
Windows multimedia scheduling to reduce display-side stalls. No fixed-rate or
minimum-PC performance guarantee has been verified with a physical card.

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

FPS test: click **FPS test** or press **F8** to measure 30 seconds of live
capture. The live readout uses recent frame intervals; the test result uses
actual counts over its full duration. The player counts capture arrivals and
frames submitted for display.
Click **Close** on the FPS overlay, or press **F8** during/after the test, to
hide it without stopping video or audio. F8 or **FPS test** starts a new test
when it is hidden. Closing a running test saves its partial results, marked
"stopped early". Completed results remain available in the report file:

  %APPDATA%\XR1 Play\windows-fps-test.txt
Use real capture with Test picture OFF for hardware testing. 59.94 fps is valid.
The report also counts late arrivals, skipped/invalid frames and audio faults,
and records GPU upload/present timings and the desktop refresh rate;
it does not measure controller latency or the console game's own frame rate.

For a device/mode report, run in a terminal:
  .\XR1-Play.exe --diagnose > xr1-diagnostics.txt 2>&1

For the optional test picture/tone:
  .\XR1-Play.exe --demo

Normal capture uses native Media Foundation; audio uses SDL/WASAPI. It requires
an advertised native 1920x1080 60/59.94 fps mode and does not interpolate 30 fps.
This Windows update prefers D3D11 and native NV12/planar formats, rearms camera
reads before decoding, and keeps only the newest pending sample. Windows audio
uses a small priming reserve (~13 ms), gentle clock correction and short fades
through signal gaps. The queue cap is 32 ms; it does not deliberately hold 32 ms
of audio. Long USB/driver outages can still interrupt sound. --capture-buffers affects Linux V4L2 only; Windows driver/source-reader
buffering is managed by Media Foundation, with the low-latency attribute enabled.

If the camera is busy, close OBS/Camera/other video apps. For silent sound check
Xbox HDMI Stereo uncompressed, the selected Windows recording endpoint, its
mute/level and Windows privacy settings. Generic device names are selectable;
the viewer does not automatically choose an unidentified microphone.

The software is unsigned: these downloads have no purchased Windows signing
certificate. SHA256SUMS accompanies this release. No blanket compatibility or
measured end-to-end latency claim is made. Report hardware tests using the
checklist in COMPATIBILITY.md.

Hardware: x86_64 PC, USB 3 data port/cable, XR1 Lite, HDMI 1080p60 SDR source,
and a 60 Hz or faster display. Integrated graphics are a target; no dedicated
encoder or AVX2 CPU is required. There is no measured minimum CPU/RAM/GPU model
yet. EVGA's Lite manual specifies USB 3 and 1080p60, not a CPU/RAM minimum.
https://www.evga.com/Support/manuals/files/141-U1-CB20-LR.pdf

Licenses and original icon sources are included. XR1 Play is MIT licensed;
third-party libraries/fonts/icons retain their own licenses.

This software is based in part on the work of the FreeType Team. SDL2_ttf uses
FreeType and HarfBuzz; the relevant notices are included with the application.

Color preference: open **Color preference** inside the app's settings screen.
Choose Xbox green, PlayStation blue, or Nintendo red, then click **Done**.
The selection previews immediately and is saved automatically across launches.
It changes the interface accent only. Esc closes the panel; Tab/arrow keys
navigate the choices and Enter selects one. Playback continues while browsing.

The original controller mark now appears in the header badge and the Windows
app/title-bar icon. The title-bar icon follows the selected accent color; the
EXE's embedded icon uses Xbox green. The large controller retains its existing
motion and the Motion toggle. The original controller geometry is unchanged.
