#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# APPIMAGETOOL may point to an installed appimagetool. In this workspace we
# extract its official AppImage so building does not require FUSE.
if [ -n "${APPIMAGETOOL:-}" ]; then
    ARCH=x86_64 "$APPIMAGETOOL" --no-appstream build/XR1LPlayer.AppDir dist/XR1-L-Player-1.5.25-Linux-x86_64.AppImage
else
    [ -f .deps/appimagetool.AppImage ] || {
        echo 'Download official appimagetool-x86_64.AppImage to .deps/appimagetool.AppImage first.' >&2; exit 1;
    }
    chmod +x .deps/appimagetool.AppImage
    .deps/appimagetool.AppImage --appimage-extract > /dev/null
    ARCH=x86_64 squashfs-root/AppRun --no-appstream build/XR1LPlayer.AppDir dist/XR1-L-Player-1.5.25-Linux-x86_64.AppImage
fi
