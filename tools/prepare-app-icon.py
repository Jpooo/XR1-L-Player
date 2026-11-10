#!/usr/bin/env python3
"""Export Windows app badges from the existing, unchanged Lucide controller SVG.
Development asset export only; the application has no SVG/Python dependency.
"""
from pathlib import Path
import ctypes as c,struct
from PIL import Image
root=Path(__file__).resolve().parents[1]
out=root/'assets/branding';out.mkdir(parents=True,exist_ok=True)
source=(root/'assets/icons/gamepad-2.svg').read_text()
geometry=source[source.index('>')+1:source.rindex('</svg>')]
rsvg=c.CDLL('librsvg-2.so.2');cairo=c.CDLL('libcairo.so.2');gobject=c.CDLL('libgobject-2.0.so.0')
rsvg.rsvg_handle_new_from_data.argtypes=[c.c_void_p,c.c_size_t,c.c_void_p];rsvg.rsvg_handle_new_from_data.restype=c.c_void_p
rsvg.rsvg_handle_render_cairo.argtypes=[c.c_void_p,c.c_void_p];rsvg.rsvg_handle_render_cairo.restype=c.c_int
cairo.cairo_image_surface_create.argtypes=[c.c_int,c.c_int,c.c_int];cairo.cairo_image_surface_create.restype=c.c_void_p
cairo.cairo_create.argtypes=[c.c_void_p];cairo.cairo_create.restype=c.c_void_p
cairo.cairo_scale.argtypes=[c.c_void_p,c.c_double,c.c_double]
cairo.cairo_surface_write_to_png.argtypes=[c.c_void_p,c.c_char_p];cairo.cairo_surface_write_to_png.restype=c.c_int
cairo.cairo_destroy.argtypes=[c.c_void_p];cairo.cairo_surface_destroy.argtypes=[c.c_void_p];gobject.g_object_unref.argtypes=[c.c_void_p]
for key,accent,ink in [('xbox','#9bf00b','#0e1805'),('playstation','#4f8ff7','#091222'),('nintendo','#f4434e','#1d0a0e')]:
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256">
<rect x="2" y="2" width="252" height="252" rx="60" fill="{accent}"/>
<g transform="rotate(-10 128 128)"><g transform="translate(16 16) scale(9.3333333333)" fill="none" stroke="{ink}" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">{geometry}</g></g>
</svg>'''
    (out/f'xr1-play-{key}.svg').write_text(svg)
    data=svg.encode();blob=c.create_string_buffer(data)
    handle=rsvg.rsvg_handle_new_from_data(blob,len(data),None);assert handle
    surface=cairo.cairo_image_surface_create(0,512,512);context=cairo.cairo_create(surface);cairo.cairo_scale(context,2,2)
    assert rsvg.rsvg_handle_render_cairo(handle,context)
    png=out/f'xr1-play-{key}.png';assert cairo.cairo_surface_write_to_png(surface,str(png).encode())==0
    cairo.cairo_destroy(context);cairo.cairo_surface_destroy(surface);gobject.g_object_unref(handle)
    image=Image.open(png).convert('RGBA').resize((256,256),Image.Resampling.LANCZOS)
    raw=image.tobytes('raw','BGRA')
    dib=struct.pack('<IiiHHIIiiII',108,256,-256,1,32,3,len(raw),2835,2835,0,0)
    dib+=struct.pack('<IIIII',0xff0000,0xff00,0xff,0xff000000,0x73524742)+bytes(48)
    (out/f'xr1-play-{key}.bmp').write_bytes(b'BM'+struct.pack('<IHHI',122+len(raw),0,0,122)+dib+raw)
    if key=='xbox':
        image.save(out/'xr1-play.ico',sizes=[(n,n) for n in (16,20,24,32,40,48,64,128,256)])
(out/'SOURCE.txt').write_text('Windows app badge composition using unchanged Lucide gamepad-2 geometry.\nLucide 0.468.0 source: ../icons/gamepad-2.svg\nISC license: ../icons/LICENSE\nBadge color/rounded background/rotation exported by tools/prepare-app-icon.py.\nNo generated replacement controller artwork is used.\n')
print('Exported themed Windows badges and a 9-size application ICO from the original controller.')
