#include "../src/video_common.hpp"
#include <cassert>
static std::vector<uint8_t> image(int width,int height){jpeg_compress_struct c{};jpeg_error_mgr error{};c.err=jpeg_std_error(&error);jpeg_create_compress(&c);unsigned char *data=nullptr;unsigned long size=0;jpeg_mem_dest(&c,&data,&size);c.image_width=width;c.image_height=height;c.input_components=3;c.in_color_space=JCS_RGB;jpeg_set_defaults(&c);jpeg_start_compress(&c,TRUE);std::vector<uint8_t> row(static_cast<size_t>(width)*3,128);while(c.next_scanline<c.image_height){JSAMPROW p=row.data();jpeg_write_scanlines(&c,&p,1);}jpeg_finish_compress(&c);std::vector<uint8_t> bytes(data,data+size);free(data);jpeg_destroy_compress(&c);return bytes;}
int main(){
    assert(rate60(60,1)&&rate60(60000,1001)&&!rate60(30,1)&&!rate60(60,0));
    Frame f;JpegDecoder decoder;for(int h:{1080,1088}){auto bytes=image(1920,h);assert(decoder.decode(bytes.data(),bytes.size(),f)&&f.data.size()==1920*1080*3);assert(std::abs(f.data[0]-128)<3);}
    auto small=image(640,480);assert(!decoder.decode(small.data(),small.size(),f));uint8_t invalid[]={0,1,2,3};assert(!decoder.decode(invalid,4,f));auto valid=image(1920,1080);assert(decoder.decode(valid.data(),valid.size(),f));
    for(auto raw:{RawFormat::YUYV,RawFormat::UYVY,RawFormat::NV12,RawFormat::I420,RawFormat::YV12}){
        bool planar=raw==RawFormat::NV12||raw==RawFormat::I420||raw==RawFormat::YV12;size_t stride=planar?1984:3904,size=stride*(planar?1620:1080);std::vector<uint8_t> bytes(size,128);
        size_t padding=raw==RawFormat::I420||raw==RawFormat::YV12?stride/2-960:stride-(planar?1920:3840);
        assert(unpackRaw(bytes.data(),size-padding,stride,raw,f));assert(!unpackRaw(bytes.data(),size-padding-1,stride,raw,f));assert(!unpackRaw(bytes.data(),size,10,raw,f));assert(f.data.size()==static_cast<size_t>(planar?1920*1620:3840*1080));
    }
    std::cout<<"PASS native shared-codec tests: 60/59.94 rate validation, JPEG decode/crop/recovery, five raw layouts, padding and byte bounds\n";
}
