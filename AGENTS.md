# Project workflow

- Build and test updates locally first. Ask the user for explicit approval before
  pushing updated source files, uploading packages, or publishing/updating a
  GitHub Release. An explicit request for a specific GitHub action authorizes
  that action; a request to fix or test the app alone does not authorize uploads.
- Do not automatically publish packages from push-triggered workflows.
- The user approved the tested Linux UI port and explicitly authorized replacing
  the two existing releases with one latest release, "XR1-L Play beta v1",
  containing the existing Windows Beta v1 and Linux 1.5.6 packages. Preserve the
  tested executable bytes. Future code/package changes still require testing
  and explicit upload approval.
- Preserve the Linux V4L2 and audio behavior when making interface changes.

- Current work: local-only 1.5.7 builds for both platforms, sharper fullscreen
  text, measured software latency comparisons, and the XR1-L Player name.
  Export the Linux AppImage inside a ZIP for USB transfer. Do not push or
  upload these new builds/source until the user explicitly authorizes it.

- Current local update: 1.5.8 fixes the FPS panel default/restart state, renderer
  reset texture caches and unconfirmed Linux frame-rate acceptance. Review and
  test both builds; keep GitHub unchanged and the Linux download in a ZIP.

- Also persist volume and mute on both builds. Merge preferences atomically so
  changing volume, color or device selection preserves the other settings.

- Local 1.5.9 recheck: retain/retry unsaved volume after transient write failure
  and suppress stale Linux capture FPS before a restarted feed receives frames.
  Keep packages/source local; preserve the prior 1.5.8 downloads.

- Local 1.5.10: smooth menu/controller animation without changing capture/audio
  timing or original logo artwork. Future logo work waits for the user photo.

- Local 1.5.11: approved XR1 Lite device logo across the poster, header and native/package icons. Solid theme backgrounds; gentle device hover/tilt. Keep all packages local.
