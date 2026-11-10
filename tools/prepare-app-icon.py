#!/usr/bin/env python3
"""Export runtime masks and native badges from the approved XR1 Lite artwork.
Development asset export only; no Python or SVG dependency at runtime.
"""
from pathlib import Path
import base64
import io
import struct
from PIL import Image, ImageDraw
root = Path(__file__).resolve().parents[1]
out = root / 'assets/branding'
source = Image.open(out / 'xr1-lite-master.png').convert('RGBA')
# Preserve the approved artwork and padding; convert format and size only.
side = max(source.size)
alpha = Image.new('L', (side, side))
alpha.paste(source.getchannel('A'), ((side-source.width)//2, (side-source.height)//2))
mask = Image.new('RGBA', (side, side), 'white')
mask.putalpha(alpha)
def bmp(path, image):
    image = image.convert('RGBA')
    raw = image.tobytes('raw', 'BGRA')
    dib = struct.pack('<IiiHHIIiiII', 108, image.width, -image.height, 1, 32, 3, len(raw), 2835, 2835, 0, 0)
    dib += struct.pack('<IIIII', 0xff0000, 0xff00, 0xff, 0xff000000, 0x73524742) + bytes(48)
    path.write_bytes(b'BM' + struct.pack('<IHHI', 122+len(raw), 0, 0, 122) + dib + raw)
bmp(root / 'assets/icons/xr1-lite.bmp', mask.resize((1024,1024), Image.Resampling.LANCZOS))
for key, accent, ink in [('xbox','#9bf00b','#0e1805'), ('playstation','#4f8ff7','#091222'), ('nintendo','#f4434e','#1d0a0e')]:
    badge = Image.new('RGBA', (512,512))
    ImageDraw.Draw(badge).rounded_rectangle((4,4,507,507), radius=120, fill=accent)
    device = Image.new('RGBA', (448,448), ink)
    device.putalpha(alpha.resize((448,448), Image.Resampling.LANCZOS))
    badge.alpha_composite(device, (32,32))
    badge.save(out / f'xr1-play-{key}.png')
    data = io.BytesIO(); badge.save(data, format='PNG')
    encoded = base64.b64encode(data.getvalue()).decode('ascii')
    (out / f'xr1-play-{key}.svg').write_text(f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="512" height="512" viewBox="0 0 512 512"><image width="512" height="512" xlink:href="data:image/png;base64,{encoded}"/></svg>\n')
    bmp(out / f'xr1-play-{key}.bmp', badge.resize((256,256), Image.Resampling.LANCZOS))
    if key == 'xbox':
        badge.save(out / 'xr1-play.ico', sizes=[(n,n) for n in (16,20,24,32,40,48,64,128,256)])
(out / 'SOURCE.txt').write_text('XR1 Lite device artwork generated from the user-provided hardware reference and approved in chat.\nMaster: xr1-lite-master.png. Format/size exports: tools/prepare-app-icon.py.\nThe same artwork supplies the menu, header, window and package icons.\nSolid theme-color backgrounds; no PLAY watermark or diagonal decoration.\nOther interface symbols retain their Lucide ISC license in ../icons/LICENSE.\n')
print('Exported XR1 Lite runtime mask, three themed badges and nine-size Windows ICO.')
