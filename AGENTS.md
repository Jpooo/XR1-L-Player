# Project workflow

- Build and test updates locally first. Ask the user for explicit approval before
  pushing updated source files, uploading packages, or publishing/updating a
  GitHub Release. An explicit request for a specific GitHub action authorizes
  that action; a request to fix or test the app alone does not authorize uploads.
- Do not automatically publish packages from push-triggered workflows.
- The user has now authorized bringing the Windows interface/features to Linux
  as a local test build. Keep the known-good Linux 1.5.0 AppImage and published
  Windows Beta v1 intact. Preserve Linux V4L2 and audio behavior while porting UI,
  themes, branding and FPS controls. Do not upload this Linux preview until the
  user explicitly approves publication after testing.
