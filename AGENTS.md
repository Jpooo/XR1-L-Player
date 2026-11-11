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

- Local 1.5.12: futuristic mouse/keyboard menu with Play, Settings and Preferences.
  Keep the exact 1.5.11 artwork and motion functions. Theme display names are
  Neon Green / Electric Blue / Crimson Red; retain legacy saved color keys.
  Remove the FPS Test feature, reports and shortcut while preserving capture
  cadence and rendering. Motion, volume and mute preferences persist. Both
  new packages stay local, with the Linux AppImage in a ZIP for USB transfer.

- Local 1.5.13 restores the live video preview in Settings during playback,
  following the direct-texture approach in 1.5.11. Use the free space in Video
  Settings; keep the 1.5.12 menus, controls, artwork, themes and FPS Test removal.
  Both updated packages remain local; preserve prior downloads.

- Local 1.5.14: show live video only in the main-menu logo panel while playback
  is active; restore the original animated logo after stopping. Remove the
  Settings preview per the user's correction. Keep all other controls and
  capture/audio behavior unchanged. Both packages remain local.

- Local 1.5.15: inset the main-menu preview and its border safely inside the
  angled panel, keeping 16:9 without cropping. Add a separate Main menu button
  to the playback toolbar; retain Settings, mute/volume, Stop and fullscreen.
  Returning to the menu keeps video/audio running. Settings has no preview.
  Keep both builds local and preserve previous downloads.

- Local 1.5.16: fill the entire main-menu panel with uniformly scaled video,
  clip it to the panel contour and draw the frame over its edges. Keep the
  Main menu button, no Settings preview, idle logo and existing playback.
  Software renderers use cached uniform subdivision to avoid large-triangle
  gaps at scaled sizes; accelerated renderers use the small native mesh.
  Both builds stay local, with previous downloads preserved.

- Release authorization: the user explicitly requested publishing both tested
  1.5.16 builds as Latest and renaming the project XR1-L Player. Include their
  exact PNG screenshot in README. Preserve tested executable/AppImage bytes.
  Future unrequested uploads still require explicit authorization.

- Repository rename completed: canonical URL is https://github.com/Jpooo/XR1-L-Player. Use this URL for future pushes and releases.

- Published with explicit user approval: XR1-L Player 1.5.16 is the Latest
  non-draft release at https://github.com/Jpooo/XR1-L-Player/releases/tag/v1.5.16.
  Both ZIPs and the standalone AppImage were uploaded via the manual approved
  workflow; GitHub's asset digests match the verified local release files.
  The exact user screenshot is in docs/screenshot.png. Native Windows CI passed
  in addition to the local checks. Future changes still require upload approval.

- Local 1.5.17: connection status and automatic reconnect for both platforms.
  Retry only the previously selected card, HDMI input and output; wait for
  missing endpoints, reject ambiguous matches, back off failed reopen attempts,
  and let Cancel/Esc or device/format changes stop recovery. Preserve fullscreen,
  saved volume/mute and the menu the user is browsing. Keep capture/audio queues,
  rendering, approved artwork and menu transitions unchanged. No new auto-start
  preference. Keep both builds local, with the Linux AppImage in a ZIP.
  Validation: 10 Linux CTest checks; packaged controls/audio/fullscreen/appearance;
  Windows cross-build, Wine UI tests and seven simulated reconnect scenarios.
  Physical XR1 Lite unplug/replug and native Windows hardware validation remain.

- Local 1.5.18: status text follows the selected theme; the separate connection
  dot stays green when connected and red when disconnected/reconnecting, across
  all themes and both platforms. Keep packages local and preserve prior builds.
