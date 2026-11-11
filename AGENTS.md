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

- Local 1.5.19: consistent approved XR1 Lite icons and sharper menus at display
  scales/resolutions through 4K. Use multi-size themed Windows icon resources,
  per-monitor DPI awareness, Wayland desktop identity and Linux hicolor exports.
  Rasterize rounded menu masks at drawable density. Keep the approved artwork,
  layout, capture/audio and video scaling unchanged; no video upscaler. Keep both
  builds local and preserve previous downloads.
  Verified: all 11 Linux checks, twelve drawable-size checks through 3840x2160,
  packaged controls/appearance, Windows icon resources and UI under Wine, and a
  fullscreen 4K virtual-display screenshot. Native Windows mixed-monitor DPI
  transitions and real Linux compositor/launcher behavior remain to be tested.

- Local 1.5.20: use the properly licensed Inter Semibold face for main navigation,
  headings and the app title, without simulated bold. Preserve body text, existing
  sizes/layout, icons, theme choices and all playback behavior. Bundle the static
  font and SIL OFL license in both packages. Keep builds local.
  Checked physical-resolution text through 4K, all 11 Linux CTest checks,
  packaged controls/appearance, Windows scaling and packaged demo under Wine.
  The Windows UI timing fixture passed when rerun without concurrent rendering
  tests; its first run missed the software preview draw-count threshold under
  shared test load. Native hardware/display validation remains with the user.

- Local 1.5.21: replace Inter headings with the restrained, squared Oxanium
  Semibold face to match the neon panel theme. Main buttons, headings and title
  change; body text, label sizes/positions and playback remain unchanged. Bundle
  the font and SIL OFL license on both platforms. Keep packages local.
  Checked native 1080p detail and physical-resolution headings through 4K,
  packaged Linux controls/appearance, and Windows scaling, full UI and packaged
  demo under Wine. Preview reviewed at the normal window size. Native Windows
  and real display/capture testing remain with the user.

- Local 1.5.22: implement the approved Orbitron-derived bold menu typography,
  centered by visible glyph bounds, with theme-colored Play, neutral secondary
  labels, shadows, and a slow highlight/glow cycle. Cache physical-resolution
  text textures; font/theme/renderer changes clear them. Motion Off is still.
  Use an RFN-compliant internal font name while preserving approved outlines.
  Preserve layout, artwork, capture/audio and video presentation. Both builds
  remain local; preserve older downloads. Future feature ideas are discussion
  only until the user requests a specific addition.
  Verified: all 12 Linux CTest checks, packaged Linux controls/audio/appearance,
  and Windows typography, scaling, complete UI and packaged demo under Wine.
  Rendering checks include normal/fractional/4K sizes, three themes, glyph-center
  clearance, warm-cache reuse, Motion Off and reset cleanup. Native Windows and
  physical capture/display testing remain with the user. ZIP/embedded binary,
  font and license hashes were verified.

- Local 1.5.24: discard the 1.5.23 local packages and restore the user-selected
  screenshot typography: original Chakra Petch Semibold 44/27, centered white
  letter faces and static soft shadows/glow. Remove the built-in test picture,
  its generated tone, settings action and CLI switch from both production apps.
  Hardware substitutes live only in test executables and are never packaged.
  Preserve native capture/audio behavior, artwork and other controls. Keep the
  new Windows ZIP and Linux AppImage ZIP local; no GitHub updates.
  Verified: all 12 Linux CTest checks; test-only capture/input controls and audio;
  shipping AppImage menus, tester-option rejection and settings persistence;
  native audio bridge under dummy audio on Linux and Wine; Windows typography,
  seven reconnect scenarios, full UI and shipping idle/help checks under Wine.
  Both production binaries lack tester/tone/fixture strings, packages exclude
  test executables, and embedded binary/font/license hashes match. Native Windows
  and real XR1 Lite hardware validation remain with the user.

- Local 1.5.25: offer YUYV/NV12 only (Automatic negotiates only those two).
  Migrate retired/invalid saved formats to Automatic and reject explicit retired
  CLI formats. Remember normal window size and fullscreen across relaunch, with
  bounded sizes, display clamping, atomic merged preferences and --windowed
  override. Review associated settings/startup behavior. Both packages local.
  Verified 13 Linux CTest checks and restricted native capture negotiation,
  controls/audio/appearance fixtures, preferences and Windows UI under Wine.
  Final startup clamping was additionally checked by resize/relaunch/fullscreen
  and oversized-saved-window tests on Linux and Wine, plus the final packaged
  AppImage menu test. ZIP/binary/documentation hashes match. Native hardware
  and Windows display/driver testing remain with the user.

- Local 1.5.26: user chose sharper fullscreen only for a 1080p Xbox feed on a
  1440p PC display. Add cached GPU sharp-bilinear fractional upscale, preserving
  native/integer/windowed/menu-preview paths, with a saved On/Off Settings
  control and graceful renderer fallback. Validate YUYV/NV12, including SDL GL
  NV12 render-target state invalidation. No added frame queue or CPU conversion.
  Keep current color conversion unchanged: generated BT.709 probes exposed a
  potential mismatch, but actual card matrix/range metadata is unverified.
  Both packages remain local; no GitHub uploads.
  Verified all 15 Linux CTest checks, software/OpenGL sharp-path readbacks for
  YUYV/NV12, native/integer bypass, renderer/filter restoration, allocation
  fallback/no retry loop, cache reuse and saved preference/live-toggle behavior.
  Final Windows scaling/preference/full-UI and shipping launch checks passed
  under Wine. An absent test display and full temporary test disk were corrected
  before the final Windows rerun; downloads were reverified afterward. Actual
  GPU/card responsiveness and picture quality still require user comparison.

- Local 1.5.27: remove the rejected sharper-fullscreen rendering, preference and
  setting from both builds; restore the previous linear video presentation.
  Remove Play/Resume/Retry and Fullscreen buttons only from Settings. Keep main
  menu Play, Preferences fullscreen, playback toolbar and keyboard shortcuts.
  Preserve all other 1.5.25 improvements and approved menus/artwork. Both builds
  stay local; export the Linux AppImage in a ZIP and preserve older downloads.
  Verified all 13 Linux CTest checks, fixture controls/audio/appearance, shipping
  AppImage menus/settings, Windows full UI and shipping launch/help under Wine.
  Read-only UI review caught and fixed remapped mute/volume focus highlights.
  ZIP CRC, embedded executable/AppImage bytes, permissions and hashes match.
  Real card/display and native Windows validation remain with the user.

- The user rejected the separate 120 Hz capture experiment and requested removal
  of its tester. Removed its isolated worktree and generated downloads; retain
  the experimental branch only as local source history, without merging.
  Regular 1.5.27 remains the active tester-free build on both platforms.
  No interpolation, refresh-rate changes or GitHub upload authorized by this
  cleanup. Explain that 120 Hz presentation of a 60 fps feed repeats frames;
  a PC refresh rate evenly divisible by 60 can improve presentation cadence.

- Local 1.5.28: optional AMD FSR 1 FP32 EASU spatial upscaling, default Off,
  without RCAS sharpening, frame generation or extra frame queues. Apply only
  to the full feed above 1080p; keep capture/audio/menu preview unchanged. Save
  the preference atomically. Enabled startup selects OpenGL 4.2 compatibility
  on both platforms; Off preserves the prior renderer selection. Windows needs
  an app restart after enabling FSR. Unsupported hardware falls back to normal
  scaling with a Settings status message. Bundle upstream MIT sources/license.
  New Windows ZIP and Linux AppImage ZIP stay local; no GitHub upload.
  Verified all 14 Linux CTest checks, updated shipping AppImage controls and
  preference persistence, GL EASU image orientation/color/state/cache/native
  bypass with YUYV and NV12, and a 4K fixture render. Windows full UI, shipping
  launch/help, actual EASU shader image checks and enabled startup passed under
  Wine. Unsupported renderer startup falls back successfully. ZIP CRC, embedded
  app/shader/license bytes, permissions and manifest hashes match. Native card,
  physical Windows GPU performance and end-to-end latency remain unverified;
  the virtual-display GPU checks use Mesa software rendering.

- Local 1.5.29: U toggles FSR during playback with a short status message and
  saved preference. Keep the shader/target cache alive across A/B toggles, but
  invalidate the cached picture so re-enabling uses the current frame. Reuse
  the same action from Settings. No capture/audio restart. The existing Windows
  renderer setup requires enabling FSR in Settings and restarting once before
  live comparison; show that requirement when U is pressed on a non-GL renderer.
  Keep packages local and previous downloads intact.
  Verified all 14 Linux checks including hotkey On/Off and preference reload;
  OpenGL YUYV/NV12 cache invalidation; four real UI hotkey toggles in one Linux
  GPU fixture session without reopening capture/audio; shipping AppImage menu
  persistence. Windows UI/hotkey, GL image/cache, startup and packaged launch
  checks passed under Wine. ZIP CRC, embedded app/shader bytes, permissions and
  SHA-256 manifests match. Physical card/native GPU testing remains with user.

- Local 1.5.30: optional AMD RCAS sharpening after FSR EASU. Default Off,
  with Off/Low/Medium/High in Settings and Shift+U cycling live. U still toggles
  FSR. Save both choices atomically. Sharpening only runs with active FSR above
  1080p; no extra RCAS pass when disabled, no temporal history/frame generation
  or changes to capture/audio. Reuse cached shaders/targets, refresh after live
  changes and fall back to EASU if RCAS is unavailable. Keep both builds local.
  Verified all 14 Linux checks, Shift+U cycling/persistence, actual GL RCAS
  Low/Medium/High differences, flat-color preservation, exact Off restoration,
  YUYV/NV12 state/cache behavior and recovery from an injected conversion
  failure. Four live sharpening changes kept one capture/audio session alive.
  Windows full UI, GL EASU/RCAS image checks and packaged launch passed under
  Wine. Shipping AppImage menus/default-Off persistence, ZIP CRC, embedded app,
  shader/license bytes, permissions and hashes match. Native GPU/card quality
  and end-to-end latency still need the user's hardware comparison.

- Beta V2 branding: retain the tested 1.5.30 behavior, app name, settings,
  dependencies and icons; show exactly v2 in the main-menu version label.
  Use Beta V2 in help/product branding and local package names; retain the
  internal 1.5.30 build identifier. User cancelled GitHub upload before any
  remote writes; build local Linux AppImage ZIP and Windows ZIP only. Do not
  push source/tags, dispatch publication workflows or create a release until
  the user gives new explicit upload approval. Preserve prior downloads.
  Verified the approved source is unchanged apart from menu/help/product
  version text; visually checked v2 in the packaged main menu. Linux packaged
  controls/persistence and Windows launch/help under Wine passed. ZIP CRC,
  embedded executables/shader/license bytes, permissions and hashes match.
  Nothing pushed, no remote branch/tag/release created or changed.

- Renewed publication authorization: user explicitly requested uploading the
  approved Beta V2 builds as a GitHub pre-release and removing the old release.
  Keep tested executable bytes intact. Refresh release documentation, upload
  both ZIPs, the AppImage and checksums, verify remote assets, publish non-draft
  with prerelease=true, then delete the superseded 1.5.16 release (keep its git
  tag/history). Update main README download links. Do not run the obsolete
  publish-approved-release workflow: it targets the superseded 1.5.16 package.

- Published with explicit renewed user authorization: XR1-L Player Beta V2 is
  a non-draft pre-release at https://github.com/Jpooo/XR1-L-Player/releases/tag/beta-v2.
  Tag beta-v2 points to tested application source 6f5cb515c05d2956219dbba380c30f580eda1c06.
  Release docs refreshed; approved Linux/Windows executable bytes unchanged.
  Direct uploads rejected the session credential, so the manual publisher used
  verified packages from pinned staging commit 63be0481b4a8b85f4545c71b4b9c8f76ecbf4a5c.
  Native Windows CI 43121917195 and publication workflow 43123351610 passed.
  All four public assets were anonymously downloaded and SHA-256 verified.
  Superseded v1.5.16 release deleted as requested; its git tag/history retained.
  Future unrequested code/package uploads still need explicit user approval.
