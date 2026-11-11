# XR1-L Player 1.5.21 — Linux

1080p60 live preview and HDMI stereo audio for EVGA XR1 Lite.

## Connect and play

1. Extract the ZIP and copy the AppImage from your USB drive to your Linux home folder.
2. In Properties → Permissions, allow the AppImage to run as a program, then open it.
3. Connect console HDMI OUT → XR1 Lite HDMI IN; connect the card directly to USB 3.0 with a USB 3.0 data cable.
4. Use 1080p60 SDR and HDMI Stereo uncompressed. Select video, HDMI audio and your speakers/headphones in Settings, then choose Play.

Use **Settings** for video/audio devices, format and volume. **Preferences** saves Neon Green, Electric Blue, Crimson Red and Motion On/Off. Choose **Play** to start the feed. Move the mouse during playback to reveal **Main menu**, Settings and audio controls.

Connection status shows detected devices, live video and waiting/reconnecting states. After a device disconnects, the app retries the selected video/audio devices automatically. Use **Cancel** or **Esc** on the main menu to stop reconnecting. Changing a device in Settings also cancels recovery.

F/F11: fullscreen; Tab: settings / return to video; Enter: stop playback; M: mute; +/-: volume. In menus use the mouse, arrows and Enter; Esc goes back. Volume and mute are saved across launches.

Current x86_64 Bazzite/Linux with glibc 2.38+ is the binary target. Without FUSE, launch with `--appimage-extract-and-run`.
Settings use `${XDG_CONFIG_HOME:-$HOME/.config}/xr1-l-player/`. Previous XR1 Play settings are imported when no new settings exist.

XR1 Lite requirements: Intel i5-6XXX or AMD Ryzen 3 1300X or better; NVIDIA GTX 1060 or AMD Radeon RX 560 XT or above; 8 GB RAM minimum (16 GB recommended). USB 3.0 is required.

Local test build; not uploaded to GitHub. Hardware unplug/replug testing is still needed.
