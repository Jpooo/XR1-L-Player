# Contributing to XR1 Play

The project is intended to remain a native, live-only capture viewer with audio.
Useful contributions include real-device compatibility reports, driver fixes,
low-latency improvements, accessibility, packaging and clearer UI behavior.

## Build and test

Start with README.md and COMPATIBILITY.md. CMake supports the shared application
and a codec/layout test. The Makefile also runs Linux V4L2 mock tests, rendering
checks, and scripted controls/audio checks. tools/build-windows.sh is the
cross-build recipe used for the current Windows preview.

Keep generated builds, binaries, downloaded dependencies and local configuration
out of source changes. Include the test commands and results with a contribution.
For capture changes, exercise both operating systems where possible and identify
which platform or hardware was unavailable.

## Code map

- src/video_common.hpp: format/rate helpers, bounded raw-frame copies, JPEG decode.
- src/video_linux.hpp: V4L2 discovery, stable paths, mode selection and capture.
- src/video_windows.hpp: native Media Foundation discovery and asynchronous capture.
- src/core.hpp: newest-frame handoff and bounded PCM queue.
- src/audio.hpp: capture/playback audio and volume handling.
- src/main.cpp and src/ui.hpp: shared application flow and SDL user interface.
- tests/: codec/layout, driver, rendering, audio and control checks.

## Capture changes

Preserve actual 1920x1080 at 60/59.94 fps and stereo audio. Do not disguise a
30 fps source as 60 fps by repeating/interpolating frames. Avoid unbounded
queues, hold the frame-sharing lock briefly, and keep hardware-specific quirks
separate from general format negotiation. Do not hard-block unknown device IDs
merely because they are absent from a tested-device list.

Hardware reports should identify the release/commit, OS, card model, firmware
if known, USB ID/speed, selected native format/rate, audio endpoint, and results
for start/stop, disconnect/reconnect and fullscreen. COMPATIBILITY.md includes
a checklist. Successful compiles, demos, Wine runs and mocks do not verify a
physical device or justify a claim about every firmware/unit.

UI changes should be checked at default/minimum sizes and fullscreen. Keep the
capture path independent of decorative animation and retain a reduced-motion
option. The existing icons are licensed Lucide assets; retain their notices
and original source when redistributing them.

XR1 Play code is MIT licensed. Third-party libraries, fonts and icons retain
their own licenses. Update relevant notices if replacing bundled dependencies.
