#pragma once
#include "video_common.hpp"
#include <fstream>
#include <map>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include <unistd.h>
#include <filesystem>
#include <thread>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <csetjmp>
#include <jpeglib.h>
inline int vio(int fd,unsigned long req,void *arg) { int r; do {r=ioctl(fd,req,arg);} while(r<0&&errno==EINTR); return r; }
inline std::string fourcc(uint32_t f) { std::string s(4,' '); for(int i=0;i<4;++i) s[i]=static_cast<char>((f>>(8*i))&255); return s; }
inline std::string readDeviceValue(const std::filesystem::path &p){std::ifstream f(p);std::string value;std::getline(f,value);return value;}
inline std::vector<VideoDevice> videoDevices() {
    std::vector<VideoDevice> result;std::error_code ec;
    std::map<std::string,std::string> stable;
    for(auto &entry:std::filesystem::directory_iterator("/dev/v4l/by-id",ec)){std::error_code e;auto target=std::filesystem::canonical(entry.path(),e);if(!e)stable[target.string()]=entry.path().string();}
    ec.clear();
    for(auto &p:std::filesystem::directory_iterator("/dev",ec)) {
        auto node=p.path().filename().string();if(node.rfind("video",0)!=0)continue;
        int fd=open(p.path().c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);if(fd<0)continue;
        v4l2_capability c{};bool capture=false;
        if(vio(fd,VIDIOC_QUERYCAP,&c)==0){uint32_t caps=(c.capabilities&V4L2_CAP_DEVICE_CAPS)?c.device_caps:c.capabilities;capture=(caps&V4L2_CAP_VIDEO_CAPTURE)&&(caps&V4L2_CAP_STREAMING);}
        close(fd);if(!capture)continue;
        std::string usb;
        auto parent=std::filesystem::canonical(std::filesystem::path("/sys/class/video4linux")/node/"device",ec);
        if(!ec)for(int i=0;i<8&&!parent.empty();++i,parent=parent.parent_path()){auto vendor=readDeviceValue(parent/"idVendor"),product=readDeviceValue(parent/"idProduct");if(!vendor.empty()&&!product.empty()){usb=vendor+":"+product;break;}}
        std::string name(reinterpret_cast<char*>(c.card),strnlen(reinterpret_cast<char*>(c.card),sizeof(c.card)));
        result.push_back({stable.count(p.path().string())?stable[p.path().string()]:p.path().string(),name,usb});
    }
    std::sort(result.begin(),result.end(),[](auto &a,auto &b){return a.path<b.path;});return result;
}
inline bool sameVideoPath(const std::string &a,const std::string &b){if(a==b)return true;std::error_code x,y;auto aa=std::filesystem::canonical(a,x),bb=std::filesystem::canonical(b,y);return !x&&!y&&aa==bb;}
inline bool jpegFormat(uint32_t f){return f==V4L2_PIX_FMT_MJPEG||f==V4L2_PIX_FMT_JPEG;}
inline bool rawFormat(uint32_t f,RawFormat &out){switch(f){case V4L2_PIX_FMT_YUYV:out=RawFormat::YUYV;break;case V4L2_PIX_FMT_UYVY:out=RawFormat::UYVY;break;case V4L2_PIX_FMT_NV12:out=RawFormat::NV12;break;case V4L2_PIX_FMT_YUV420:out=RawFormat::I420;break;case V4L2_PIX_FMT_YVU420:out=RawFormat::YV12;break;default:return false;}return true;}
inline bool requestFormat(const std::string &request,uint32_t f){return request=="auto"||request==lower(fourcc(f))||(request=="mjpeg"&&jpegFormat(f))||(request=="i420"&&f==V4L2_PIX_FMT_YUV420)||(request=="yv12"&&f==V4L2_PIX_FMT_YVU420);}
inline bool configureRate60(int fd,uint32_t pixel,double &fps){
    std::vector<v4l2_fract> advertised;bool fixed=false,conflicting=false;
    for(unsigned i=0;i<128;++i){v4l2_frmivalenum v{};v.index=i;v.pixel_format=pixel;v.width=1920;v.height=1080;if(vio(fd,VIDIOC_ENUM_FRAMEINTERVALS,&v)<0)break;if(v.type!=V4L2_FRMIVAL_TYPE_DISCRETE)break;advertised.push_back(v.discrete);}
    fixed=advertised.size()==1&&rate60(advertised[0].denominator,advertised[0].numerator);
    std::vector<v4l2_fract> attempts={{1,60}};for(auto t:advertised)if(rate60(t.denominator,t.numerator))attempts.push_back(t);
    for(auto rate:attempts){
        v4l2_streamparm set{};set.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;set.parm.capture.timeperframe=rate;
        bool accepted=vio(fd,VIDIOC_S_PARM,&set)==0;
        v4l2_streamparm get{};get.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bool reported=vio(fd,VIDIOC_G_PARM,&get)==0;
        auto reportedRate=get.parm.capture.timeperframe;
        if(reported&&reportedRate.numerator&&reportedRate.denominator&&!rate60(reportedRate.denominator,reportedRate.numerator))conflicting=true;
        auto t=reported&&reportedRate.numerator&&reportedRate.denominator?reportedRate:set.parm.capture.timeperframe;
        if(((reported&&reportedRate.numerator&&reportedRate.denominator)||accepted)&&rate60(t.denominator,t.numerator)){fps=double(t.denominator)/t.numerator;return true;}
    }
    if(fixed&&!conflicting){auto t=advertised.front();fps=double(t.denominator)/t.numerator;return true;}
    return false;
}
class Capture {
    struct Mapping { void *p; size_t size; };
    int fd=-1;
    std::vector<Mapping> buffers;
    std::thread worker;
    std::atomic<bool> running{false};
    SharedVideo &shared;
    uint32_t format=0;
    unsigned stride=0;
    bool streaming=false;
    void run() {
        JpegDecoder jpeg; Frame next;
        double last=monotime(),lastValid=last;
        while(running) {
            pollfd p{fd,POLLIN,0}; int pr=poll(&p,1,100);
            if(pr<0) { if(errno==EINTR) continue; shared.fail("Video poll failed: "+std::string(strerror(errno))); break; }
            if(pr==0) { if(monotime()-last>5) {shared.fail("No video frames for 5 seconds. Check HDMI, USB 3 and Xbox 1080p/60 settings.");break;} continue; }
            if(p.revents&(POLLERR|POLLHUP|POLLNVAL)) {shared.fail("Capture card disconnected or stopped streaming.");break;}
            // Hold only the newest completed buffer; release stale buffers before decoding.
            v4l2_buffer newest{}; bool have=false;
            for(size_t attempt=0;attempt<buffers.size();++attempt) {
                v4l2_buffer b{}; b.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; b.memory=V4L2_MEMORY_MMAP;
                if(vio(fd,VIDIOC_DQBUF,&b)<0) {
                    if(errno!=EAGAIN) shared.fail("Dequeue failed: "+std::string(strerror(errno)));
                    break;
                }
                if(b.index>=buffers.size()) { shared.fail("Driver returned an invalid buffer index."); break; }
                if(have) { vio(fd,VIDIOC_QBUF,&newest); ++shared.skipped; }
                newest=b; have=true;
            }
            if(!have) continue;
            last=monotime();
            const uint8_t *src=static_cast<uint8_t*>(buffers[newest.index].p);
            bool valid=!(newest.flags&V4L2_BUF_FLAG_ERROR) && newest.bytesused<=buffers[newest.index].size;
            if(valid&&jpegFormat(format))valid=jpeg.decode(src,newest.bytesused,next);
            else if(valid){RawFormat raw;valid=rawFormat(format,raw)&&unpackRaw(src,newest.bytesused,stride,raw,next);}
            next.received=monotime();
            if(vio(fd,VIDIOC_QBUF,&newest)<0) {shared.fail("Requeue failed: "+std::string(strerror(errno)));break;}
            if(valid){lastValid=monotime();shared.publish(next);}else {++shared.corrupt;if(monotime()-lastValid>5){shared.fail("Capture is returning invalid frames. Check USB 3 bandwidth, HDMI signal and the selected pixel format.");break;}}
        }
    }
public:
    explicit Capture(SharedVideo &s):shared(s) {}
    ~Capture() { stop(); }
    void start(const std::string &path,const std::string &requested,unsigned bufferCount=2) {
        stop();
        if(bufferCount<2||bufferCount>8)throw std::runtime_error("Capture buffers must be 2..8.");
        {
            std::lock_guard<std::mutex> lock(shared.mutex);
            shared.error.clear(); shared.dirty=false;
            shared.captured=0; shared.skipped=0; shared.corrupt=0;
        }
        std::vector<uint32_t> choices;
        fd=open(path.c_str(),O_RDWR|O_NONBLOCK|O_CLOEXEC);
        if(fd<0)throw std::runtime_error("Cannot open "+path+": "+strerror(errno)+". Close other capture apps and check device permissions.");
        for(auto candidate:{V4L2_PIX_FMT_YUYV,V4L2_PIX_FMT_NV12}){
            if(!requestFormat(requested,candidate))continue;
            for(unsigned i=0;i<128;++i){v4l2_fmtdesc d{};d.index=i;d.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;if(vio(fd,VIDIOC_ENUM_FMT,&d)<0)break;if(d.pixelformat==candidate)choices.push_back(candidate);}
        }
        close(fd);fd=-1;std::string failures;
        for(auto candidate:choices){
            fd=open(path.c_str(),O_RDWR|O_NONBLOCK|O_CLOEXEC);if(fd<0)break;
            try {
                v4l2_format f{};f.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;f.fmt.pix.width=1920;f.fmt.pix.height=1080;f.fmt.pix.pixelformat=candidate;f.fmt.pix.field=V4L2_FIELD_NONE;
                if(vio(fd,VIDIOC_S_FMT,&f)<0||f.fmt.pix.width!=1920||f.fmt.pix.height!=1080||f.fmt.pix.pixelformat!=candidate||(f.fmt.pix.field!=V4L2_FIELD_NONE&&f.fmt.pix.field!=V4L2_FIELD_ANY))throw std::runtime_error("1080p progressive mode unavailable");
                double fps=0;if(!configureRate60(fd,candidate,fps))throw std::runtime_error("60/59.94 fps could not be confirmed");
                format=candidate;stride=f.fmt.pix.bytesperline;
                v4l2_requestbuffers req{};req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;req.memory=V4L2_MEMORY_MMAP;
                bool allocated=false;
                for(unsigned count=bufferCount;count<=std::max(4u,bufferCount);++count){req.count=count;if(vio(fd,VIDIOC_REQBUFS,&req)==0&&req.count>=2&&req.count<=64){allocated=true;break;}}
                if(!allocated)throw std::runtime_error("driver rejected capture buffers");
                for(unsigned i=0;i<req.count;++i){
                    v4l2_buffer b{};b.index=i;b.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;b.memory=V4L2_MEMORY_MMAP;
                    if(vio(fd,VIDIOC_QUERYBUF,&b)<0||!b.length)throw std::runtime_error("cannot query capture buffer");
                    void *p=mmap(nullptr,b.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,b.m.offset);if(p==MAP_FAILED)throw std::runtime_error("cannot map capture buffer");buffers.push_back({p,b.length});
                    if(vio(fd,VIDIOC_QBUF,&b)<0)throw std::runtime_error("cannot queue capture buffer");
                }
                v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE;if(vio(fd,VIDIOC_STREAMON,&type)<0)throw std::runtime_error("stream start failed: "+std::string(strerror(errno)));
                streaming=true;
                std::ostringstream mode;mode<<"1920 x 1080 / "<<fps<<" fps / "<<fourcc(format)<<" / "<<buffers.size()<<" driver buffers";shared.mode=mode.str();
                running=true;worker=std::thread(&Capture::run,this);return;
            }catch(const std::exception &e){failures+=fourcc(candidate)+": "+e.what()+". ";stop();}
        }
        throw std::runtime_error("No working 1920x1080 at 60/59.94 fps mode. "+failures+"Use USB 3 and Xbox 1080p/60 SDR. Check --diagnose and EVGA's firmware if 1080p60 is not advertised.");
    }
    void stop() {
        running=false; if(worker.joinable()) worker.join();
        if(fd>=0) { if(streaming) {v4l2_buf_type t=V4L2_BUF_TYPE_VIDEO_CAPTURE; vio(fd,VIDIOC_STREAMOFF,&t);} for(auto b:buffers) munmap(b.p,b.size); buffers.clear(); close(fd); fd=-1; }
        streaming=false;
    }
};
inline void diagnoseVideo() {
    auto devices=videoDevices();
    if(devices.empty()) std::cout<<"No accessible V4L2 capture devices. Check USB connection and permissions.\n";
    for(auto &d:devices) {
        std::cout<<d.path<<" : "<<d.name<<" / USB "<<d.usbId<<"\n"; int fd=open(d.path.c_str(),O_RDWR|O_NONBLOCK|O_CLOEXEC); if(fd<0) continue;
        for(unsigned i=0;;++i) {
            v4l2_fmtdesc fmt{}; fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; fmt.index=i; if(vio(fd,VIDIOC_ENUM_FMT,&fmt)<0) break;
            std::cout<<"  "<<fourcc(fmt.pixelformat)<<" "<<fmt.description<<"\n";
            for(unsigned j=0;;++j) {
                v4l2_frmivalenum v{}; v.index=j; v.pixel_format=fmt.pixelformat; v.width=1920; v.height=1080; if(vio(fd,VIDIOC_ENUM_FRAMEINTERVALS,&v)<0) break;
                if(v.type==V4L2_FRMIVAL_TYPE_DISCRETE) { if(v.discrete.numerator) std::cout<<"    1920x1080 @ "<<static_cast<double>(v.discrete.denominator)/v.discrete.numerator<<" fps\n"; }
                else {std::cout<<"    1920x1080 variable frame intervals\n";break;}
            }
        }
        close(fd);
    }
}
struct CapturePlatform {};
