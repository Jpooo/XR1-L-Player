# XR1-L Player 1.5.28 — Windows

1080p60 live preview and HDMI stereo audio for EVGA XR1 Lite.

## Connect and play

1. Extract the whole ZIP, then open XR1-L-Player.exe. Keep DLLs and assets together.
2. Connect console HDMI OUT → XR1 Lite HDMI IN; connect the card directly to USB 3.0 with a USB 3.0 data cable.
3. Use 1080p60 SDR and HDMI Stereo uncompressed. Allow desktop camera and microphone access in Windows Settings.
4. Select video, HDMI audio and your speakers/headphones in Settings, then choose Play from the main menu.

Requires Windows 10/11 x64 with Media Foundation; Windows N needs the Media Feature Pack.
XR1 Lite requirements: Intel i5-6XXX or AMD Ryzen 3 1300X or better; NVIDIA GTX 1060 or AMD Radeon RX 560 XT or above; 8 GB RAM minimum (16 GB recommended). USB 3.0 is required.

Use **Settings** for video/audio devices, format and volume. **Preferences** saves Neon Green, Electric Blue, Crimson Red and Motion On/Off. Choose **Play** to start the feed. Move the mouse during playback to reveal **Main menu**, Settings and audio controls.

Connection status shows detected devices, live video and waiting/reconnecting states. After a device disconnects, the app retries the selected video/audio devices automatically. Use **Cancel** or **Esc** on the main menu to stop reconnecting. Changing a device in Settings also cancels recovery.

F/F11: fullscreen; Tab: settings / return to video; Enter: stop playback; M: mute; +/-: volume. In menus use the mouse, arrows and Enter; Esc goes back. Volume and mute are saved across launches.
Settings use %APPDATA%\XR1-L Player\. Previous XR1 Play settings are imported when no new settings exist.

Local test build; not uploaded to GitHub. Hardware unplug/replug testing is still needed.

Windowed size and fullscreen state are restored on launch. `--windowed` overrides saved fullscreen. Automatic format selection tries only YUYV and NV12.

Optional upscaling: Settings → Upscaling → FSR 1 EASU. Default Off; sharpening is disabled. Windows requires an app restart after enabling it to select the OpenGL renderer. It applies to the full video feed when displayed larger than 1080p. The main-menu preview is unchanged. Unsupported GPUs/renderers use normal scaling. FSR processes each frame independently; it does not increase capture frame rate or restore lost source detail. GPU cost and responsiveness need comparison on your hardware.
