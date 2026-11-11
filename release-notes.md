XR1-L Player Beta V2 brings the current Linux and Windows builds together, with **v2** shown in the menu. Native 1080p60 video and HDMI stereo audio for EVGA XR1 Lite.

- Connection status and automatic reconnect, plus saved volume, mute, window size and fullscreen.
- Optional AMD FSR 1 upscaling and Off/Low/Medium/High sharpening. Both default to Off.
- **U** toggles FSR; **Shift+U** cycles sharpening while playing.

**Linux:** download the Linux ZIP containing the AppImage, extract it and allow it to run as a program. **Windows:** extract the entire Windows ZIP and open `XR1-L-Player.exe`; keep its DLLs and assets together. USB 3.0 is required.

FSR applies to video displayed above 1080p. On Windows, enable FSR in Settings and restart before live comparison. The program binaries match the locally tested Beta V2 build; release documentation has been refreshed.

This is a pre-release for testing. Physical capture-card and GPU behavior can vary across systems.

Validation: 14 Linux checks, local Windows UI/shader checks under Wine, and [native Windows CI](https://github.com/Jpooo/XR1-L-Player/actions/runs/43121917195) passed.
