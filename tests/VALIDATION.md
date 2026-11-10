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

## Windows 1.5.2 local test build (not published)

The user reports capture near 60 fps with about 58 shown in 1.5.1. This update
alternates two reusable GPU upload destinations and scopes MMCSS Games/high
scheduling to active playback. A failed scheduling request uses normal priority;
failed second-texture allocation retains a single-texture fallback. No extra
video frame queue is introduced. The Windows audio path is unchanged.

The live FPS readout now uses up to 120 measured frame intervals. This removes
the sampling-phase mismatch between received counts and the previous submitted
frame count. The 30-second test still uses actual frame counts and elapsed time.
The report includes upload/present call timings, slow calls, scheduling mode,
upload destination count, and the desktop refresh rate reported by SDL.

- Warning-free native PE cross-build of the app, render test and capture test.
- Native PE render test passed under Wine with newest-frame pixel readback for
  NV12, IYUV, YV12, YUY2, UYVY and RGB24; alternating texture ownership, reset,
  scheduling cleanup and accurate 60-versus-58 rate/report regression cases.
- Native MF callback and Windows jitter/clock-drift/audio tests passed under Wine.
- Final 30-second generated-picture run under Wine: 60.02 received / 59.98
  submitted fps, zero skipped/invalid frames, upload call mean/max 0.99/7.45 ms,
  present call mean/max 0.23/2.41 ms. Latest interval-based readouts were about
  60.04 received / 60.05 shown. These are software checks, not a card benchmark.
- Windows ZIP checked for corruption, exe identity and all bundled dependencies.
- Linux main/audio preprocessor output and AppImage checksum remain unchanged.

No GitHub upload or Release mutation was performed. Real Windows 11 capture,
GPU behavior and any throughput improvement require another hardware test.

## Windows 1.5.3 local UI update (not published)

Added a Close button on the FPS overlay and an F8 toggle for running/completed
tests. Closing finishes an active measurement, saves a report marked stopped
early, and hides the overlay. Playback/capture/audio are not stopped. F8 or the
FPS test button starts a fresh measurement after closing.

The Windows portable package compiled and passed a three-second Wine smoke
run with 180 received/displayed frames and no skipped/invalid frames. Screenshot
inspection confirmed the Close button is visible without overlapping the FPS
text; early-stop report output was verified. Linux preprocessed main code and
the Linux AppImage checksum are unchanged. No GitHub files were uploaded.

## Windows 1.5.4 local appearance update (not published)

Added an in-app Color preference panel: Xbox green, PlayStation blue and
Nintendo red. The choice previews immediately, saves across launches and does
not change capture/audio/device settings. The existing dashboard layout,
controller icon, player controls and capture paths remain. Windows buttons use
more restrained highlights and borders. No native window menu bar was added.

- Warning-free Windows PE build.
- Windows preference regression executable passed under Wine: all three choices
  reload correctly, existing device/format selections and unknown settings fields
  survive color changes, and a normal device-settings save retains the color.
- Rendered and inspected the dashboard and preference panel for all three themes.
  Selected state, swatches, readable contrast and panel spacing were checked.
- Packaged Windows playback smoke check under Wine: 240 received/displayed
  generated frames over four seconds, no skipped/invalid frames. This does not
  establish physical capture performance.
- ZIP integrity, executable identity and bundled dependencies checked.
- Linux main/UI preprocessor output and AppImage checksum remain unchanged.

The user wants these additions carried to Linux after the Windows build is
finished. That port has not begun. No GitHub source/package upload was performed.

## Windows 1.5.5 local branding placement (not published)

The original controller mark replaces the XR initials inside the header badge.
The same unchanged Lucide geometry is framed as a rounded Windows app badge,
embedded in the EXE at nine sizes (16 through 256 px), and loaded as the native
window/title-bar icon. Runtime window icons follow the selected accent color;
the file icon uses Xbox green. The large controller's existing bob/tilt motion
and reduced-motion behavior are unchanged. No rejected generated replacement
artwork was incorporated.

- Original controller SVG/BMP bytes verified unchanged against the prior commit.
- Warning-free native Windows PE build including the resource object.
- EXE resources verified: nine RT_ICON entries and an RT_GROUP_ICON entry.
- All ICO sizes and themed BMP transparency verified.
- Windows dashboard screenshots rendered under Wine for Xbox and PlayStation;
  the original controller header mark is correctly placed and readable.
- Portable ZIP integrity, embedded executable identity and all icon assets checked.
- Linux main/UI preprocessor output and AppImage checksum remain unchanged.
- No GitHub source/package upload was performed.


## Linux 1.5.6 local UI port (not published)

Ported the completed Windows in-app color preferences, original controller
branding, refined buttons, live FPS readout and closable 30-second FPS test.
The selected color persists across launches. F8 or Close saves a partial report
and hides the FPS panel while playback continues. The Linux report is
`~/.config/xr1-play/fps-test.txt` (or the XDG_CONFIG_HOME equivalent).

Linux retains its existing V4L2 backend, texture upload path, audio ring with a
20 ms queue cap and 128-sample audio callback. Windows capture/scheduling/audio
workarounds were not applied to Linux. New cadence counters measure actual
frame events; generated-picture tests are not hardware or input-latency tests.

Validation:

- Optimized Linux build; all three CMake/CTest targets passed: codec/layouts,
  saved preferences and FPS state/cadence.
- Mock V4L2 capture suite passed, including padded formats, frame-rate selection,
  buffer handling, error/disconnect handling and JPEG crop cases.
- Accelerated renderer upload/readback passed for YUY2, NV12, IYUV, YV12 and
  UYVY, with linear scaling verified; latest-frame handoff checks passed.
- Both native and packaged control checks passed: volume/mute, fullscreen,
  settings/back-to-game, motion, all three color selections, preference save,
  Escape, F8 restart/close and the visible FPS Close button. Audio-device hooks
  verified that opening/closing these panels did not stop playback. PCM checks
  verified stereo output, gain and the unchanged callback period.
- Packaged 30-second generated test: 60.02 received / 59.99 submitted fps,
  zero skipped/invalid frames and zero dropped/starved audio samples. This is a
  software smoke check, not XR1 Lite hardware performance validation.
- Dashboard screenshots checked with the Linux X11 renderer and all themes.
- AppImage/tar package includes branding assets and the Linux instructions.
- Original Linux 1.5.0 AppImage and Windows 1.5.5 ZIP checksums remain unchanged.

The source is saved on local branch `linux-local-preview`; no source, package,
branch or release was uploaded to GitHub. Real Bazzite/XR1 Lite playback and
sound require user testing before publication.


## XR1-L Player 1.5.7 local test build

Fullscreen menu text now aligns to physical pixels and preserves the original font raster. Native 1080p single-pixel video detail is unchanged and exact. MJPEG chroma upsampling is smoothed; its test-fixture MSE falls from 27.49 to 23.91, with a measured Linux decode cost of about 0.32 ms. Ready frames wake the UI directly; live diagnostic text updates at 4 Hz while counts remain per-frame.

Controlled handoff comparisons (median of three run means): Linux 0.1132 → 0.0328 ms; Windows under Wine 0.1248 → 0.0957 ms. Windows p95 worsened from 0.2065 to 0.2500 ms, so no confirmed native Windows latency gain is claimed. These exclude HDMI/monitor/game/controller delay. The separate local comparison report contains methods and raw timing context.

Linux capture, four CMake tests, packaged controls/audio/appearance, software/OpenGL pixel fidelity and six Windows checks under Wine passed. Names and metadata use XR1-L Player; legacy settings are retained/read as fallback. Audio buffering and capture negotiation are unchanged. New AppImage/Windows ZIPs are local only; Linux is additionally wrapped in a ZIP for USB transport. No remote updates were made.


## XR1-L Player 1.5.8 local FPS/default and code-review fixes

The FPS panel now defaults closed and resets on every new feed. Restarted feeds reset shown-frame totals/timing summaries alongside capture counts. Renderer resets clear all cached UI textures as well as video uploads. Linux no longer accepts an empty rate-query response as confirmation of a failed 60 fps request; unambiguous fixed-60 fallback remains supported. FPS report writes flush before success is reported and surface failures in the settings error area. The UI header now explicitly includes filesystem.

Five native Linux CMake tests and seven Windows executables under Wine passed. The real player UI tests check hidden startup, explicit/F8 open, F8 close, stop/restart, renderer-reset recovery and report-write/flush failure. Native V4L2 mocks and address/undefined-behavior sanitizers with leak checking passed. Packaged controls, appearance/audio checks, Windows playback and startup screenshot pixel checks passed. There is no physical capture device or native Windows GPU available; these are software checks, not hardware certification. Both packages remain local; the Linux AppImage is also exported in a ZIP.

Volume/mute persistence was also added to this local 1.5.8 build. Shared preference writes now merge device/theme/volume fields and atomically replace the settings file after a successful write/close. Tests cover 0/100% levels, mute/unmute, invalid numeric values, legacy migration, cross-preference preservation and failed-write preservation. Actual player relaunch checks verify the restored slider and mute indicator on both platforms. Separate packaged processes verified 50% PCM gain on Linux and a restored 37% slider with muted PCM output on Windows under Wine. All five Linux tests and seven Windows check programs pass.


## XR1-L Player 1.5.9 local follow-up check

Two new regression cases failed on 1.5.8 and pass after fixes: failed volume saves remain pending and retry (with a two-second backoff and final exit attempt), and Linux capture FPS is zero before the first received frame following a restart. The volume test deliberately causes one save failure, then verifies the corrected value while the player is still running.

Five Linux tests, seven Windows executables under Wine, mocked native capture/codec/audio checks and a Linux real-player UI run with address/undefined-behavior sanitizers and leak detection passed. Packaged Linux controls/appearance/audio and Windows playback smoke checks passed. No physical capture or native Windows GPU is available. Prior downloads are preserved; no remote source/package/release updates were made.


## XR1-L Player 1.5.10 local smooth-menu update

Idle menu animation follows the current display refresh (24–240 Hz, 60 Hz fallback) instead of a fixed 30 fps. A fixed-phase menu clock prevents accumulating timer oversleep and skips missed deadlines without catch-up queues. Settings shown during capture remain capped at 60 Hz; Motion Off retains 20 Hz idle updates. Controller bob and button lift use fractional coordinates while text stays aligned to physical pixels. Original artwork, motion amplitude/speed, audio, V4L2/Media Foundation, upload destinations, native wake path and vsync preference are unchanged.

Six Linux CMake targets passed, including new menu timing/fractional-coordinate/Motion Off checks. Windows motion, presentation, player UI and preference checks passed under Wine. OpenGL readback verified real subpixel controller movement and retained exact fullscreen font-raster fidelity. Packaged control/appearance/audio and Windows playback checks passed. Virtual-host timing is diagnostic only; native Windows/GPU smoothness still requires the user's hardware test. No source or packages were uploaded. New logo work waits for the user's reference photo.

## XR1-L Player 1.5.11 local device-logo update

Replaced the controller with the user-approved XR1 Lite artwork in the menu tile,
header badge, three themed window icons, Windows executable resource/ICO, and
Linux AppImage desktop icon. The tile has a solid preference color without the
PLAY watermark or diagonal stripes. The device floats by 2 logical pixels with
at most 0.8 degrees of tilt; Motion Off holds it level and stationary. Capture,
audio, video scaling, saved preferences and presentation scheduling are unchanged.

Validation: Make capture/audio/codec checks passed; all six Linux CTest checks
passed. Accelerated Linux readback preserved native 1080p video pixels and the
physical-resolution text raster (mean/max error 0/0); logo subpixel movement was
0.221 pixels for a requested 0.25-pixel step. Normal and reduced menu motion
passed on Linux and Windows. Windows presentation and actual-player FPS/volume
start/restart/persistence/retry tests passed under Wine using the new packaged
assets. The Windows package itself launched and produced the expected menu.
Packaged AppImage controls and appearance checks passed, including audio volume,
fullscreen, theme selection and FPS Close/reopen. Both ZIPs passed CRC checks;
embedded executable/artwork bytes match the build outputs. AppImage executable
permissions are retained inside its ZIP.

Windows checks used Wine, not a physical Windows 11 machine or XR1 Lite. These
UI tests do not measure capture-card throughput or physical end-to-end latency.
Both 1.5.11 ZIPs are local only; prior downloads and GitHub releases are unchanged.

## XR1-L Player 1.5.12 local menu redesign

The main menu has Play, Settings and Preferences, with neon navigation, neutral
theme swatches, curved accent lines and a panel fitted around the 1.5.11 device
artwork. Settings separates video and audio controls. Preferences exposes Neon
Green, Electric Blue and Crimson Red with the original numeric colors and legacy
saved keys; Motion On/Off is saved through the existing merged settings writer.
Menus support mouse, arrows, Enter, Escape and keyboard focus. Playback shortcuts
and fullscreen remain. Tab opens Settings during playback and returns to video;
Escape returns from a submenu to the main menu without stopping playback.

Removed the diagnostic class/Windows alias, 30-second test/report lifecycle,
report writes, F8 binding, CLI option and related UI. Normal frame cadence,
newest-frame handoff and generic console timing counters remain. Production
Linux and Windows executables were scanned for the removed feature strings.

Preservation checks: SHA-256 matches the prior 1.5.11 files for the logo master,
menu icon mask, motion implementation, frame cadence, V4L2/Media Foundation
capture, audio pipelines and Windows video presenter. The artwork is not redrawn,
stretched or cropped. Its original dark ink sits over a silhouette-fitting accent
backing, drawn with the same bob and tilt; no square image tile is visible.
Only menu rendering/layout and the removed diagnostic change the playback loop.
No controller detection or controller-button prompts were introduced.

Validation:
- Make capture/codec/audio checks passed; six Linux CTest checks passed.
- Real-player input checks passed for mouse/keyboard menu navigation, format
  cycling, three themes, saved motion/volume/mute, transient volume-save retry,
  clean feed start, stop/restart, Settings/resume, preferences during playback,
  renderer reset, and inactive removed shortcut. Screenshots checked at the
  minimum 900x560 window and 1600x900, with aspect-preserving logical scaling.
- Accelerated Linux presentation preserved native 1080p single-pixel detail;
  text error against physical-resolution raster was mean/max 0/0. Logo
  fractional movement and Motion Off passed on Linux accelerated and Windows.
- Six rebuilt Windows checks passed under Wine: motion, real UI, preference
  persistence, audio buffering, video presenter/cadence and presentation fidelity.
- Packaged AppImage tests passed for volume/audio, six fullscreen toggles,
  settings/resume, all themes, motion and uninterrupted test-tone audio through
  menu navigation. The packaged Windows EXE launched and produced the new menu.
- Visual inspection covered the main menu, Settings, Preferences, minimum-size
  layout and packaged Windows menu. ZIP CRC/permissions/executable identity
  and preserved logo bytes are checked at export.

No physical XR1 Lite or native Windows 11 hardware was available. The test picture
and Wine validate UI/control behavior, not real hardware throughput, latency or
synchronization. All 1.5.12 files are local only; previous downloads and GitHub
releases remain unchanged.

## XR1-L Player 1.5.13 local Settings preview restoration

Compared 1.5.11's Settings preview with 1.5.12 and restored its direct drawing of
an existing video texture. The new 192x108 (16:9) preview occupies unused space
below Video format, beside Test picture. It appears only while playback has a
texture. Existing controls and their hit areas are unchanged. Newly displayed
preview frames count as presented, as in 1.5.11. No additional capture path,
frame queue or CPU video-copy stage was introduced; full-feed rendering,
capture/audio engines, themes, artwork and motion code are unchanged.

All six Linux CTest checks passed. Real-player tests verified moving preview
pixels (74 draws / 3 sampled values on this run), minimum-window rendering,
Settings reentry/resume and no preview before playback. The corresponding
rebuilt Windows UI suite passed under Wine (79 draws / 2 sampled values),
including the existing navigation, preference and volume-save/retry checks.
Screenshots were visually checked on Linux and Windows. A test of the actual
Linux AppImage showed 114 frames received/displayed while Settings stayed open;
stereo PCM output continued at the expected gain. These are software-fixture
checks, not real hardware performance measurements. No XR1 Lite or native
Windows 11 hardware was available. Export ZIPs are checked for CRC, executable
identity and Linux executable permissions. No GitHub files/releases changed.

## XR1-L Player 1.5.14 local main-menu preview

Moved live video into the main-menu logo panel and removed it from Settings,
per the user's follow-up. While playing, the existing texture draws into a
fixed 544x306 (16:9) area inside the unchanged panel rails. Without active video,
the same original logo mask, colors and motion functions draw as before. Stopped
playback never passes its retained video texture to the panel, avoiding stale
preview frames. Controls, hit areas, capture/audio code, video presenter, artwork
and logo motion are unchanged. This uses the existing texture and adds no frame
queue, separate capture path or CPU video-copy stage.

All six Linux CTest checks passed. Real-player readback confirmed changing source
pixels on the main menu (100 draws / 3 sampled values), no logo over live video,
logo restoration after stop, main-menu resume, the minimum window size and no
preview in Settings. The rebuilt Windows UI suite passed under Wine (71 main
preview draws / 2 sampled values), including the same switch/stop/resume checks
and the existing preference, volume, restart and reset checks. Screenshots were
inspected for live video, the restored logo and empty Settings preview area.
The actual AppImage kept stereo PCM output at the expected gain while displaying
video on the main menu. Software fixtures do not verify physical XR1 Lite or
native Windows performance. Both packages stay local; prior downloads and GitHub
releases are untouched. Export ZIPs are checked for CRC, executable identity and
Linux executable permissions.

## XR1-L Player 1.5.15 preview fit and playback Main menu button

Inset the video to a centered 512x288 rectangle with clearance for its border
inside all angled panel rails. The full 16:9 image is preserved. Added a distinct
Main menu button to the playback toolbar, retaining Settings and all playback
controls. Shared layout constants drive button drawing, hit areas and slider
mapping. Returning through Main menu leaves capture/audio active; Play resumes
the feed. Settings remains without a preview; stopping restores the original
logo. Capture/audio, video presenter, logo assets and motion code are unchanged.

Six Linux CTest checks passed. The rebuilt Windows UI suite passed under Wine,
including the new Main menu click, menu-to-feed resume, retained Settings button,
changing live-preview pixels, stop-to-logo behavior and a 1920x1080 layout check.
Screenshots were checked at 1080p and the default window size for preview/panel
clearance and toolbar labels. Packaged AppImage controls and appearance checks
passed with the relocated slider/mute/Settings hit areas, including gain, stereo
output, six fullscreen toggles, theme saving and motion. A packaged Main menu
button check verifies continuing video and stereo PCM during the transition.
The original minimum-window, saved volume/mute/retry and renderer-reset checks
also passed. No physical capture-card/native Windows performance test was
possible here. Both ZIPs remain local; GitHub and prior downloads are unchanged.

## XR1-L Player 1.5.16 panel-filling preview

The preview now fills the actual angled/notched panel outline. Uniform centered
cover scaling retains the source proportions; the frame masks the video's edges
and corners. Textured geometry samples the existing capture texture directly,
with panel edges and rails drawn afterward. No additional video capture path,
offscreen video buffer or CPU frame-copy stage was introduced. Full-feed
rendering, capture/audio engines, idle logo/motion, toolbar Main menu and absent
Settings preview remain unchanged.

Visual inspection caught missing large triangles in SDL's software renderer at
fractional window scales. The final implementation uniformly subdivides and
caches the software mesh according to physical scale, avoiding both lost areas
and mismatched shared edges. Accelerated renderers keep the original small mesh.
Tests now sample triangle interiors densely and check masked corners at both
1100x650 and 1920x1080, with static and streaming textures.

All eight Linux CTest checks passed. Panel coverage/cutout checks passed for RGB24,
YUY2, UYVY, NV12, IYUV and YV12 on software and accelerated OpenGL, including
scaled windows and streaming textures. Rebuilt Windows normal/scaled panel tests
and the real-player UI suite passed under Wine. Real-player checks covered moving
video, the Main menu button, resume, logo restoration after stop, minimum/1080p
windows and no Settings preview. Packaged AppImage controls and appearance checks
passed for volume/stereo output, fullscreen, theme/motion persistence and menu
navigation. Final scaled and OpenGL screenshots were inspected for filled
interiors and frame overlap. ZIP CRC, executable identity and Linux executable
permissions are checked at export.

No physical XR1 Lite or native Windows 11 hardware was available; these checks do
not measure hardware capture throughput or end-to-end latency. Both local builds
and source remain unuploaded; GitHub and prior downloads are unchanged.
