#!/usr/bin/env python3
"""Rasterize pinned Lucide SVGs with librsvg into alpha BMPs SDL can load natively.
Run only when updating the icon assets; normal builds require no SVG libraries.
"""
from pathlib import Path
import ctypes as c,struct,urllib.request
root=Path(__file__).resolve().parents[1]
folder=root/'assets/icons';folder.mkdir(parents=True,exist_ok=True)
base='https://raw.githubusercontent.com/lucide-icons/lucide/0.468.0/'
rsvg=c.CDLL('librsvg-2.so.2');cairo=c.CDLL('libcairo.so.2');gobject=c.CDLL('libgobject-2.0.so.0')
rsvg.rsvg_handle_new_from_data.argtypes=[c.c_void_p,c.c_size_t,c.c_void_p];rsvg.rsvg_handle_new_from_data.restype=c.c_void_p
rsvg.rsvg_handle_render_cairo.argtypes=[c.c_void_p,c.c_void_p];rsvg.rsvg_handle_render_cairo.restype=c.c_int
cairo.cairo_image_surface_create.argtypes=[c.c_int,c.c_int,c.c_int];cairo.cairo_image_surface_create.restype=c.c_void_p
cairo.cairo_create.argtypes=[c.c_void_p];cairo.cairo_create.restype=c.c_void_p
cairo.cairo_scale.argtypes=[c.c_void_p,c.c_double,c.c_double]
cairo.cairo_image_surface_get_data.argtypes=[c.c_void_p];cairo.cairo_image_surface_get_data.restype=c.c_void_p
for method in ('cairo_surface_flush','cairo_destroy','cairo_surface_destroy'):getattr(cairo,method).argtypes=[c.c_void_p]
gobject.g_object_unref.argtypes=[c.c_void_p]
for name in ('monitor','volume-2','volume-x','gamepad-2','maximize','refresh-cw','play','square','chevron-right','check'):
    source=urllib.request.urlopen(base+'icons/'+name+'.svg',timeout=20).read()
    (folder/(name+'.svg')).write_bytes(source)
    data=source.replace(b'currentColor',b'#ffffff');blob=c.create_string_buffer(data)
    handle=rsvg.rsvg_handle_new_from_data(blob,len(data),None);assert handle,name
    size=512 if name=='gamepad-2' else 128
    surface=cairo.cairo_image_surface_create(0,size,size);context=cairo.cairo_create(surface);cairo.cairo_scale(context,size/24,size/24)
    assert rsvg.rsvg_handle_render_cairo(handle,context),name
    cairo.cairo_surface_flush(surface)
    raw=bytearray(c.string_at(cairo.cairo_image_surface_get_data(surface),size*size*4))
    # The SVG is monochrome. Unpremultiply its white RGB, retaining Cairo's AA alpha.
    for i in range(0,len(raw),4):raw[i:i+3]=b'\xff\xff\xff'
    dib=struct.pack('<IiiHHIIiiII',108,size,-size,1,32,3,len(raw),2835,2835,0,0)
    dib+=struct.pack('<IIIII',0xff0000,0xff00,0xff,0xff000000,0x73524742)+bytes(48)
    assert len(dib)==108
    bmp=b'BM'+struct.pack('<IHHI',122+len(raw),0,0,122)+dib+raw
    (folder/(name+'.bmp')).write_bytes(bmp)
    cairo.cairo_destroy(context);cairo.cairo_surface_destroy(surface);gobject.g_object_unref(handle)
(folder/'LICENSE').write_bytes(urllib.request.urlopen(base+'LICENSE',timeout=20).read())
(folder/'SOURCE.txt').write_text('Lucide 0.468.0\nhttps://github.com/lucide-icons/lucide/tree/0.468.0/icons\nMonochrome raster exports from the included SVG sources. Original geometry unchanged.\nLicense: ISC (see LICENSE).\n')
print('Prepared 10 Lucide icons with transparency; no new application runtime dependencies.')
