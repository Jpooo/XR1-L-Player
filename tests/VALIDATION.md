# XR1 Play 1.5.0 validation

Build environment: Debian 13, Linux x86_64. No physical capture card is attached.
Tests establish software behavior, not compatibility with every unit/firmware.
See ../COMPATIBILITY.md for the hardware checklist and platform boundary.

## Linux

- Warning-free GCC 14 optimized build and CMake Release build.
- CTest shared codec/layout test with assertions enabled, including in Release.
- Mock V4L2 integration for YUYV, NV12, UYVY, I420/YU12, YV12, MJPEG and JPEG.
- Verified 60 and 60000/1001 acceptance and 30 fps rejection.
- Optional frame-rate ioctl cases: G_PARM absent, both rate ioctls absent with
  one fixed advertised rate, conflicting/ambiguous rate reports rejected.
- Two-buffer request, a driver minimum of three, rejected two-buffer request
  retried with three, explicit three-buffer request.
- Raw stream-start failure falls back to eligible MJPEG mode in Auto.
- Row/plane padding, zero default stride, truncated final padding tail, exact
  active-byte boundaries, short/invalid input rejection and reconnect handling.
- Valid/malformed/wrong-size JPEG handling and recovery, including a 1920x1088
  coded image cropped to the negotiated logical 1920x1080 frame.
- Capture tests passed AddressSanitizer/UndefinedBehaviorSanitizer checks
  (leak detection disabled). These checks do not cover a live kernel driver.
- GPU upload/render/readback for YUY2, NV12, IYUV, YV12 and UYVY with padded
  strides, plus linear interpolation of a scaled edge, under Xvfb.
- Actual SDL audio capture-to-playback callbacks through disk-backed PCM,
  stereo equality, signal amplitude and 440 Hz tone checks in earlier tests;
  audio queue overflow/underflow/mute cases continue passing in this release.
- Packaged AppImage exercised volume, audio amplitude at 50%, mute/unmute,
  header/toolbar fullscreen buttons, F11, double-click, Escape, Settings,
  Back to game and motion toggle. 241 generated frames received/displayed
  during four seconds; no invalid frames. This is not a physical card test.
- Portable .run help launch and AppImage extraction-mode launch passed.
  FUSE mounting itself is not tested in this cloud environment.

## Windows x64 preview

- Compiled a native Windows GUI-subsystem PE executable using MinGW GCC 14.2; Media Foundation
  video backend with native subtype/rate selection, asynchronous sample delivery,
  latest-frame handoff, invalid/no-frame monitoring and shutdown cleanup.
- Recursively checked all non-system DLL imports and bundled the required four
  DLLs. The package does not bundle or require Wine for Windows use.
- Ran the actual Windows executable under Wine 10. Help, Media Foundation startup,
  menu launch and bundled asset loading passed. The portable folder was launched
  from a different working directory to verify assets are resolved by exe path.
- The Windows binary generated/received/displayed 180 test frames in three seconds.
  Its Windows SDL playback callbacks wrote equal stereo PCM channels at 440 Hz,
  expected amplitude (peak 1259 at the default gain), with no queue overflow.
- A separately compiled native Windows shared-codec/layout test ran under Wine
  and passed rate, JPEG crop/recovery, five raw layouts, padding and bounds cases
  using the Windows libjpeg-turbo 3.2.1 DLL.
- The final GUI-subsystem release also passed a two-second playback smoke run.
- Wine execution is an emulated software check, not a Windows hardware/camera test.

## UI and release limits

The headline and subtitle are absent in shared code, and the menu uses a compact
1100x650 logical layout on both platforms. The neutral header, device selection,
preview, icons, volume, fullscreen and motion controls remain. Linux screenshots
and a native Windows-binary screenshot confirm the removed text is absent.

Real XR1 Lite USB transfer, HDMI/audio capture on Windows, every firmware, OS
permission/profile combinations, HDR/HDCP behavior and measured end-to-end latency
have not been verified here. Earlier Linux versions worked on the user's Bazzite
machine, but this release still needs that hardware regression test.

Build/test entry points: Makefile, CMakeLists.txt, tests/capture_test.cpp,
tests/common_test.cpp, tests/render_test.cpp, tests/controls_test.py,
and tools/build-windows.sh. SDK/library notices accompany the release folders.

## Windows 1.5.1 preview update

Windows-only changes: asynchronous callback rearm before worker decoding;
latest-sample bound with COM ownership and clean shutdown; effective native
60/59.94 mode verification; D3D11 and native planar-format preference; primed
clock-tracking audio with short fades; on-screen 30-second FPS test/report.

- Native Windows MF callback regression test passed under Wine: no inline
  decoding before rearm, newest pending sample retained, sample lifetime after
  callback return, stream ticks, stopped callbacks and failed reads.
- Windows audio regression test passed both as a host test and as a native PE
  under Wine: 120-second simulations at -300/0/+300 ppm with uneven 480-frame
  input and 256-frame output callbacks, no overflow/underrun, stereo equality,
  bounded signal edges, starvation recovery, mute ramp and overflow handling.
- FPS counters distinguish 60 from 30 fps and label test-picture runs.
- A 30-second Windows PE playback/FPS-test smoke run under Wine reported 60.02
  received / 59.98 submitted fps, with no skipped or invalid frames; the report
  file and overlay were verified. This uses generated video, not capture hardware.
- Linux main/audio preprocessor output matches the previous release. The Linux
  AppImage was not rebuilt or replaced. No Linux video backend changes.

The user's Windows 11 hardware report for 1.5.0 confirms basic capture, with
uneven video timing and intermittent audio crackle. These fixes need a new
physical XR1 Lite / Windows test; no minimum CPU/GPU model or end-to-end latency
has been established by the software tests.
