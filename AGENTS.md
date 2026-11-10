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
