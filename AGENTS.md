# Project workflow

- Build and test updates locally first. Ask the user for explicit approval before
  pushing updated source files, uploading packages, or publishing/updating a
  GitHub Release. An explicit request for a specific GitHub action authorizes
  that action; a request to fix or test the app alone does not authorize uploads.
- Do not automatically publish packages from push-triggered workflows.
- Preserve the working Linux 1.5.0 AppImage and Linux runtime behavior unless
  the user explicitly asks for Linux changes. Windows fixes and FPS testing
  remain Windows-only for now. The user wants the finished Windows additions
  ported to Linux after the Windows build is finished; do not begin that port
  during the current Windows UI iterations.
