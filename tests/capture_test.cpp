#include "../src/video.hpp"
#include "../src/audio.hpp"
#include <cstdarg>
#include <cassert>
#include <array>
static uint32_t mockFormat=V4L2_PIX_FMT_YUYV;
static int mockFps=60;
static bool fractional=false;
static size_t mockBytes=0,mockStride=0,mockMap=0;
static std::atomic<bool> disconnected{false};
static bool emptyGetRate=false,noGetRate=false,noSetRate=false,rejectTwo=false,multiFormat=false,failRawStream=false,zeroStride=false;
static int advertised=0;static uint32_t selectedFormat=0;
static unsigned mockMinimumBuffers=2,mockActualBuffers=2;
static std::array<bool,3> queued{},ready{};
extern "C" int __wrap_ioctl(int,unsigned long request,...) {
    va_list args;va_start(args,request);void *arg=va_arg(args,void*);va_end(args);
    switch(request) {
    case VIDIOC_ENUM_FMT: {auto &f=*static_cast<v4l2_fmtdesc*>(arg);if(multiFormat){if(f.index>1)break;f.pixelformat=f.index?V4L2_PIX_FMT_MJPEG:V4L2_PIX_FMT_YUYV;}else {if(f.index)break;f.pixelformat=mockFormat;}return 0;}
    case VIDIOC_S_FMT: {auto &f=*static_cast<v4l2_format*>(arg);if(!multiFormat&&f.fmt.pix.pixelformat!=mockFormat)break;selectedFormat=f.fmt.pix.pixelformat;f.fmt.pix.bytesperline=zeroStride?0:mockStride;return 0;}
    case VIDIOC_G_PARM: {if(noGetRate)break;auto &p=*static_cast<v4l2_streamparm*>(arg);p.parm.capture.timeperframe=emptyGetRate?v4l2_fract{0,0}:fractional?v4l2_fract{1001,60000}:v4l2_fract{1,static_cast<unsigned>(mockFps)};return 0;}
    case VIDIOC_S_PARM:if(noSetRate)break;return 0;
    case VIDIOC_ENUM_FRAMEINTERVALS: {auto &v=*static_cast<v4l2_frmivalenum*>(arg);if(!advertised||v.index>=static_cast<unsigned>(advertised))break;v.type=V4L2_FRMIVAL_TYPE_DISCRETE;v.discrete={1,advertised==2&&v.index==0?30u:60u};return 0;}
    case VIDIOC_REQBUFS: {auto &r=*static_cast<v4l2_requestbuffers*>(arg);if(rejectTwo&&r.count==2)break;mockActualBuffers=std::max(mockMinimumBuffers,r.count);r.count=mockActualBuffers;queued={};ready={};return 0;}
    case VIDIOC_QUERYBUF: {auto &b=*static_cast<v4l2_buffer*>(arg);b.length=mockMap;b.m.offset=b.index*mockMap;return 0;}
    case VIDIOC_QBUF:queued[static_cast<v4l2_buffer*>(arg)->index]=true;return 0;
    case VIDIOC_DQBUF: {auto &b=*static_cast<v4l2_buffer*>(arg);for(size_t i=0;i<3;++i)if(ready[i]){ready[i]=false;queued[i]=false;b.index=i;b.bytesused=mockBytes;return 0;}errno=EAGAIN;return -1;}
    case VIDIOC_STREAMON:if(failRawStream&&!jpegFormat(selectedFormat)){errno=ENOSPC;return -1;}return 0;
    case VIDIOC_STREAMOFF:return 0;
    }
    errno=EINVAL;return -1;
}
extern "C" int __wrap_poll(pollfd *fds,nfds_t,int) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    if(disconnected) {fds[0].revents=POLLHUP;return 1;}
    ready=queued;fds[0].revents=POLLIN;return 1;
}
static std::vector<uint8_t> makeJpeg(int width=1920,int height=1080) {
    jpeg_compress_struct c{};jpeg_error_mgr error{};c.err=jpeg_std_error(&error);jpeg_create_compress(&c);
    unsigned char *data=nullptr;unsigned long size=0;jpeg_mem_dest(&c,&data,&size);
    c.image_width=width;c.image_height=height;c.input_components=3;c.in_color_space=JCS_RGB;jpeg_set_defaults(&c);jpeg_set_quality(&c,90,TRUE);jpeg_start_compress(&c,TRUE);
    std::vector<uint8_t> row(static_cast<size_t>(width)*3,128);
    while(c.next_scanline<c.image_height) {JSAMPROW p=row.data();jpeg_write_scanlines(&c,&p,1);}
    jpeg_finish_compress(&c);std::vector<uint8_t> result(data,data+size);free(data);jpeg_destroy_compress(&c);return result;
}
static void waitFor(SharedVideo &s) {for(int i=0;i<100&&s.captured<4;++i)std::this_thread::sleep_for(std::chrono::milliseconds(5));assert(s.captured>=4);}
int main() {
    AudioRing ring(8);int16_t a[]={1,2,3,4,5,6},b[]={7,8,9,10,11,12},out[12];
    ring.push(a,6);ring.push(b,6);assert(ring.queued()==8);assert(ring.dropped==4);ring.pull(out,12,1);for(int i=0;i<8;++i)assert(out[i]==i+5);for(int i=8;i<12;++i)assert(out[i]==0);assert(ring.starved==4);
    ring.push(a,6);ring.pull(out,2,0);assert(out[0]==0&&out[1]==0);ring.clear();assert(!ring.queued());
    int16_t big[12];for(int i=0;i<12;++i)big[i]=i;ring.push(big,12);ring.pull(out,8,1);for(int i=0;i<8;++i)assert(out[i]==i+4);
    JpegDecoder decoder;Frame frame;auto jpeg=makeJpeg();assert(decoder.decode(jpeg.data(),jpeg.size(),frame));assert(frame.data.size()==1920*1080*3&&std::abs(frame.data[0]-128)<3);
    auto small=makeJpeg(640,480);assert(!decoder.decode(small.data(),small.size(),frame));uint8_t junk[]={0,1,2,3};assert(!decoder.decode(junk,4,frame));assert(decoder.decode(jpeg.data(),jpeg.size(),frame));
    char path[]="/tmp/xr1-fixture-XXXXXX";int fd=mkstemp(path);assert(fd>=0);
    for(uint32_t fmt:{V4L2_PIX_FMT_YUYV,V4L2_PIX_FMT_NV12}) {
        mockFormat=fmt;bool planar=fmt==V4L2_PIX_FMT_NV12||fmt==V4L2_PIX_FMT_YUV420||fmt==V4L2_PIX_FMT_YVU420;mockStride=planar?1984:3904;
        mockBytes=jpegFormat(fmt)?jpeg.size():mockStride*(planar?1620:1080);
        mockMap=(mockBytes+4095)/4096*4096;assert(ftruncate(fd,static_cast<off_t>(mockMap*3))==0);
        for(int i=0;i<3;++i) {std::vector<uint8_t> bytes=jpegFormat(fmt)?jpeg:std::vector<uint8_t>(mockBytes,static_cast<uint8_t>(i+1));assert(pwrite(fd,bytes.data(),bytes.size(),static_cast<off_t>(i*mockMap))==static_cast<ssize_t>(bytes.size()));}
        SharedVideo shared;Capture capture(shared);capture.start(path,"auto");waitFor(shared);capture.stop();assert(mockActualBuffers==2);assert(shared.skipped>=4);assert(shared.error.empty());assert(shared.frame.data.size()==(jpegFormat(fmt)?1920*1080*3:planar?1920*1620:3840*1080));if(!jpegFormat(fmt))assert(shared.frame.data[0]==2);
        mockMinimumBuffers=3;capture.start(path,"auto");waitFor(shared);capture.stop();assert(mockActualBuffers==3);if(!jpegFormat(fmt))assert(shared.frame.data[0]==3);mockMinimumBuffers=2;
        capture.start(path,"auto",3);waitFor(shared);capture.stop();assert(mockActualBuffers==3);
        // A successful reconnect clears old disconnect errors and stale frames.
        shared.error="old disconnect";capture.start(path,"auto");waitFor(shared);capture.stop();assert(shared.error.empty());
        mockFps=30;bool refused=false;try{capture.start(path,"auto");}catch(const std::runtime_error &){refused=true;}assert(refused);mockFps=60;
        fractional=true;capture.start(path,"auto");waitFor(shared);capture.stop();fractional=false;
        std::cout<<"PASS capture "<<fourcc(fmt)<<": two-buffer capture, driver minimum, explicit three buffers, latest frame, stride, reconnect, rejects 30 fps\n";
    }
    // Drivers may support only a subset of the optional frame-rate ioctls.
    mockFormat=V4L2_PIX_FMT_YUYV;mockStride=3840;mockBytes=3840*1080;mockMap=(mockBytes+4095)/4096*4096;
    assert(ftruncate(fd,static_cast<off_t>(mockMap*3))==0);std::vector<uint8_t> rawInput(mockBytes,128);
    for(int i=0;i<3;++i)assert(pwrite(fd,rawInput.data(),rawInput.size(),static_cast<off_t>(i*mockMap))==static_cast<ssize_t>(rawInput.size()));
    SharedVideo compatibility;Capture card(compatibility);
    noGetRate=true;card.start(path,"auto");waitFor(compatibility);card.stop();
    noSetRate=true;advertised=1;card.start(path,"auto");waitFor(compatibility);card.stop();
    advertised=2;bool refused=false;try{card.start(path,"auto");}catch(const std::runtime_error &){refused=true;}assert(refused);
    noGetRate=false;mockFps=30;advertised=1;refused=false;try{card.start(path,"auto");}catch(const std::runtime_error &){refused=true;}assert(refused);
    mockFps=60;emptyGetRate=true;advertised=0;
    refused=false;try{card.start(path,"auto");}catch(const std::runtime_error &){refused=true;}assert(refused);
    advertised=2;refused=false;try{card.start(path,"auto");}catch(const std::runtime_error &){refused=true;}assert(refused);
    advertised=1;card.start(path,"auto");waitFor(compatibility);card.stop(); // Fixed 60 fps remains valid evidence.
    emptyGetRate=false;noSetRate=false;mockFps=60;advertised=0;
    rejectTwo=true;card.start(path,"auto");waitFor(compatibility);card.stop();assert(mockActualBuffers==3);rejectTwo=false;
    multiFormat=true;failRawStream=true;refused=false;try{card.start(path,"auto");}catch(const std::runtime_error &){refused=true;}assert(refused);multiFormat=false;failRawStream=false;
    card.start(path,"auto");waitFor(compatibility);disconnected=true;for(int i=0;i<100;++i){{std::lock_guard<std::mutex> lock(compatibility.mutex);if(!compatibility.error.empty())break;}std::this_thread::sleep_for(std::chrono::milliseconds(5));}card.stop();assert(!compatibility.error.empty());disconnected=false;
    auto padded=makeJpeg(1920,1088);assert(decoder.decode(padded.data(),padded.size(),frame));assert(frame.data.size()==1920*1080*3);
    for(auto raw:{RawFormat::YUYV,RawFormat::UYVY,RawFormat::NV12,RawFormat::I420,RawFormat::YV12}){
        bool planar=raw==RawFormat::NV12||raw==RawFormat::I420||raw==RawFormat::YV12;size_t pitch=planar?1984:3904;size_t bytes=pitch*(planar?1620:1080);std::vector<uint8_t> data(bytes,128);
        assert(unpackRaw(data.data(),bytes,pitch,raw,frame));size_t lastPadding=raw==RawFormat::I420||raw==RawFormat::YV12?pitch/2-960:pitch-(planar?1920:3840);
        assert(unpackRaw(data.data(),bytes-lastPadding,pitch,raw,frame));assert(!unpackRaw(data.data(),bytes-lastPadding-1,pitch,raw,frame));assert(!unpackRaw(data.data(),bytes,10,raw,frame));
        size_t compact=planar?1920*1620:3840*1080;assert(unpackRaw(data.data(),compact,0,raw,frame));assert(frame.data.size()==compact);
    }
    std::cout<<"PASS optional rate ioctls, fixed-rate evidence, conflicting rate rejection, buffer retry, stream/format fallback, disconnect, zero stride, row padding bounds, 1088 JPEG crop\n";
    close(fd);unlink(path);
    std::cout<<"PASS audio queue: overflow, underrun, mute, newest samples\nPASS JPEG: 1080p decode, malformed input, wrong size, recovery\n";
}
