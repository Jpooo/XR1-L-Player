# XR1-L Play beta v1 — Linux

1080p60 live preview and stereo HDMI audio for EVGA XR1 Lite. This build
brings the Windows beta interface to Linux.

## Launch

Make the AppImage executable, then open it:

```sh
chmod +x XR1-L-Play-beta-v1-Linux-x86_64.AppImage
./XR1-L-Play-beta-v1-Linux-x86_64.AppImage
```

Without FUSE, add `--appimage-extract-and-run`. Current Bazzite/x86_64 Linux
with glibc 2.38+ and normal display/audio libraries is the binary target.

## Connect and play

1. Connect console HDMI OUT to XR1 Lite HDMI IN and the card directly to USB 3.
2. Set the console to 1080p, 60 Hz, SDR and HDMI Stereo uncompressed. Close other capture apps.
3. Select the card video device, its HDMI audio input and speakers/headphones.
4. Click **Start playing**. Leave **Test picture** off for real capture.

Use **Color preference** for saved Xbox green, PlayStation blue or Nintendo red.
**F/F11** toggles fullscreen, **Tab** opens settings, **M** mutes and **+/-** changes volume.

**FPS test / F8** measures 30 seconds. **Close** or F8 hides the overlay without
stopping playback; closing early saves a partial report. Reports are saved to
`${XDG_CONFIG_HOME:-$HOME/.config}/xr1-play/fps-test.txt`; preferences use `settings`
in the same folder. Linux counts valid/latest V4L2 capture frames. Driver-discarded
or stale frames are reported as skipped; this is not a controller-latency test.
