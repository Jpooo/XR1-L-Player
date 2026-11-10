# XR1-L Player 1.5.8 — Windows local test build

1080p60 live preview and HDMI stereo audio for EVGA XR1 Lite.

## Connect and play

1. Extract the whole ZIP, then open XR1-L-Player.exe. Keep DLLs and assets together.
2. Connect console HDMI OUT → XR1 Lite HDMI IN; connect the card directly to USB 3.0 with a USB 3.0 data cable.
3. Use 1080p60 SDR and HDMI Stereo uncompressed. Allow desktop camera and microphone access in Windows Settings.
4. Select video, HDMI audio and your speakers/headphones, then Start playing. Leave Test picture off for real capture.

Requires Windows 10/11 x64 with Media Foundation; Windows N needs the Media Feature Pack.
XR1 Lite requirements: Intel i5-6XXX or AMD Ryzen 3 1300X or better; NVIDIA GTX 1060 or AMD Radeon RX 560 XT or above; 8 GB RAM minimum (16 GB recommended). USB 3.0 is required.

Color preference saves Xbox green, PlayStation blue or Nintendo red. F/F11: fullscreen; Tab: settings; M: mute; +/-: volume. F8 starts/closes the 30-second FPS test without stopping playback. The FPS panel starts closed on each feed. Volume and mute state are saved across launches.
Settings and FPS reports use %APPDATA%\XR1-L Player\. Previous XR1 Play settings are imported when no new settings exist.

This local test build has not been uploaded to GitHub. Windows checks under Wine do not establish native Windows capture performance or controller-to-screen latency.
