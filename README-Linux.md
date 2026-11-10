# XR1-L Player 1.5.7 — Linux local test build

1080p60 live preview and HDMI stereo audio for EVGA XR1 Lite.

## Connect and play

1. Extract the ZIP and copy the AppImage from your USB drive to your Linux home folder.
2. In Properties → Permissions, allow the AppImage to run as a program, then open it.
3. Connect console HDMI OUT → XR1 Lite HDMI IN; connect the card directly to USB 3.0 with a USB 3.0 data cable.
4. Use 1080p60 SDR and HDMI Stereo uncompressed. Select video, HDMI audio and your speakers/headphones, then Start playing.

Color preference saves Xbox green, PlayStation blue or Nintendo red. F/F11: fullscreen; Tab: settings; M: mute; +/-: volume. F8 starts/closes the 30-second FPS test without stopping playback.

Current x86_64 Bazzite/Linux with glibc 2.38+ is the binary target. Without FUSE, launch with `--appimage-extract-and-run`.
Settings and FPS reports use `${XDG_CONFIG_HOME:-$HOME/.config}/xr1-l-player/`. Previous XR1 Play settings are imported when no new settings exist.

XR1 Lite requirements: Intel i5-6XXX or AMD Ryzen 3 1300X or better; NVIDIA GTX 1060 or AMD Radeon RX 560 XT or above; 8 GB RAM minimum (16 GB recommended). USB 3.0 is required.

This local test build has not been uploaded to GitHub. Software timing comparisons do not measure controller-to-screen latency.
