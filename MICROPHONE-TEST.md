# XR1-L Player — local microphone experiment (1.5.44)

This is a separate local test build based on V2 1.5.38. It does not replace the
GitHub release. Native XR1 Lite capture, game audio, FSR and artwork are unchanged.

1. Extract the whole Windows ZIP, or extract the Linux ZIP and allow its AppImage
   to run as a program. Launch XR1-L Player.
2. Enable **Remote features** on the Xbox and connect the PC and Xbox to your
   network. Keep your controller connected to the console.
3. Open **Settings → Xbox microphone**. Your microphone stays connected to the PC.
4. Choose **Sign in to Xbox**. Enter the displayed code at Microsoft's sign-in
   page. The **Microsoft sign-in** button opens that page only when you choose it;
   you can instead enter the code in an existing browser or on your phone.
5. Choose the Xbox console and PC microphone, then **Connect microphone**.
6. Return to your game. **F9** mutes/unmutes the Xbox microphone. Discord can
   continue using the same USB microphone independently.

The microphone starts only when you choose Connect. Disconnect stops its audio
capture; closing the app also stops the background connection. Account sign-in is
remembered using OS encryption where available. Without a secure Linux keyring,
sign-in lasts for this session only. Sign out removes the saved account token.

There is no second gameplay window. A hidden Remote Play session remains active
to send voice; its received game audio/video are not played or displayed. Its
video bitrate is limited, but the Xbox may still encode/send a background stream.
The XR1 Lite remains the source of the picture and game audio you use.

The status verifies outgoing microphone RTP packets. It cannot prove that a game
has accepted the mic or that other players heard it. Please test actual in-game
voice chat, not only a party or voice message. Incoming game-chat routing depends
on Xbox/game audio settings; this prototype does not play Remote Play's incoming
audio, so check that you can also hear other players.

Local checks cover hidden WebRTC microphone capture and RTP transmission,
mute/unmute/disconnect, OAuth cancellation/backoff, helper-process failure,
native menu navigation, and V2 regressions. Real Xbox sign-in/game chat and the
Windows WebRTC helper still require testing on your hardware.

No packages or source from this experiment have been uploaded to GitHub.
