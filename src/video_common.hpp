#pragma once
#include "core.hpp"
#include <filesystem>
#include <thread>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <array>
#include <csetjmp>
#include <jpeglib.h>
struct VideoDevice {std::string path,name,usbId;};
inline bool isXr1(const VideoDevice &d){return xrname(d.name)||lower(d.usbId)=="3842:310a";}
inline bool rate60(unsigned numerator,unsigned denominator){if(!numerator||!denominator)return false;double fps=double(numerator)/denominator;return fps>=59.9&&fps<=60.1;}
enum class RawFormat {YUYV,UYVY,NV12,I420,YV12,RGB24,BGR24,BGRA32};
// Copy only active pixels. A driver's padding after the final row need not be
// counted in bytesused; padding between planes/rows is still checked.
inline bool unpackRaw(const uint8_t *src,size_t size,size_t stride,RawFormat format,Frame &out){
    size_t active=format==RawFormat::NV12||format==RawFormat::I420||format==RawFormat::YV12?1920:format==RawFormat::RGB24||format==RawFormat::BGR24?5760:format==RawFormat::BGRA32?7680:3840;
    if(!stride)stride=active;
    if(stride<active||stride>65536)return false;
    bool planar=format==RawFormat::NV12||format==RawFormat::I420||format==RawFormat::YV12;
    size_t compact=active*1080+(planar?1920*540:0);
    size_t required=stride*1079+active;
    if(planar){if(format==RawFormat::NV12)required=stride*1619+1920;else {if(stride%2)return false;required=stride*1080+(stride/2)*1079+960;}}
    if(required>size)return false;
    out.pitch=static_cast<int>(active);out.data.resize(compact);
    out.pixel=format==RawFormat::YUYV?SDL_PIXELFORMAT_YUY2:format==RawFormat::UYVY?SDL_PIXELFORMAT_UYVY:format==RawFormat::NV12?SDL_PIXELFORMAT_NV12:format==RawFormat::I420?SDL_PIXELFORMAT_IYUV:format==RawFormat::YV12?SDL_PIXELFORMAT_YV12:format==RawFormat::RGB24?SDL_PIXELFORMAT_RGB24:format==RawFormat::BGR24?SDL_PIXELFORMAT_BGR24:SDL_PIXELFORMAT_BGRA32;
    for(size_t y=0;y<1080;++y)std::memcpy(out.data.data()+y*active,src+y*stride,active);
    if(planar){if(format==RawFormat::NV12){for(size_t y=0;y<540;++y)std::memcpy(out.data.data()+1920*1080+y*1920,src+stride*1080+y*stride,1920);}else {for(size_t plane=0;plane<2;++plane)for(size_t y=0;y<540;++y)std::memcpy(out.data.data()+1920*1080+plane*960*540+y*960,src+stride*1080+plane*(stride/2)*540+y*(stride/2),960);}}
    return true;
}
struct JpegError { jpeg_error_mgr base; jmp_buf jump; };
inline void jpegFail(j_common_ptr c) { longjmp(reinterpret_cast<JpegError*>(c->err)->jump,1); }
class JpegDecoder {
    jpeg_decompress_struct c{};
    JpegError error{};
    std::array<uint8_t,5760> cropRow{};
public:
    JpegDecoder() { c.err=jpeg_std_error(&error.base); error.base.error_exit=jpegFail; if(setjmp(error.jump))throw std::runtime_error("Cannot initialize JPEG decoder");jpeg_create_decompress(&c); }
    ~JpegDecoder() { jpeg_destroy_decompress(&c); }
    bool decode(const uint8_t *bytes,size_t size,Frame &out) {
        if(setjmp(error.jump)) { jpeg_abort_decompress(&c); return false; }
        jpeg_mem_src(&c,bytes,size);
        if(jpeg_read_header(&c,TRUE)!=JPEG_HEADER_OK || c.image_width!=1920 || (c.image_height!=1080&&c.image_height!=1088)) { jpeg_abort_decompress(&c); return false; }
        c.out_color_space=JCS_RGB; c.dct_method=JDCT_IFAST; c.do_fancy_upsampling=TRUE;
        jpeg_start_decompress(&c);
        if(c.output_width!=1920 || (c.output_height!=1080&&c.output_height!=1088) || c.output_components!=3) { jpeg_abort_decompress(&c); return false; }
        out.pixel=SDL_PIXELFORMAT_RGB24; out.pitch=5760; out.data.resize(5760*1080);
        while(c.output_scanline<c.output_height) { JSAMPROW row=c.output_scanline<1080?out.data.data()+c.output_scanline*5760:cropRow.data(); jpeg_read_scanlines(&c,&row,1); }
        jpeg_finish_decompress(&c); return true;
    }
};
