// Controlled software benchmark. No HDMI, scanout or controller-latency claim.
#ifndef BENCH_HEADER
#define BENCH_HEADER "../src/video_common.hpp"
#endif
#include BENCH_HEADER
#include <fstream>
#include <iterator>
static std::vector<uint8_t> read(const char *path){std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Cannot read fixture");return {std::istreambuf_iterator<char>(in),{}};}
template<class F> static double medianMs(F work,int repeats){std::vector<double> timings;for(int batch=0;batch<7;++batch){double start=monotime();for(int i=0;i<repeats;++i)work();timings.push_back((monotime()-start)*1000/repeats);}std::sort(timings.begin(),timings.end());return timings[3];}
int main(int argc,char **argv){
    if(argc!=4)return 2;
    auto jpeg=read(argv[1]),reference=read(argv[2]);Frame frame;JpegDecoder decoder;
    std::vector<uint8_t> packed(3840*1080,128),planar(1920*1620,128);
    auto yuyv=medianMs([&](){if(!unpackRaw(packed.data(),packed.size(),3840,RawFormat::YUYV,frame))std::abort();},80);
    auto nv12=medianMs([&](){if(!unpackRaw(planar.data(),planar.size(),1920,RawFormat::NV12,frame))std::abort();},80);
    auto decode=medianMs([&](){if(!decoder.decode(jpeg.data(),jpeg.size(),frame))std::abort();},12);
    if(frame.data.size()!=reference.size())return 3;
    double error=0;for(size_t i=0;i<reference.size();++i){double d=int(frame.data[i])-int(reference[i]);error+=d*d;}error/=reference.size();
    auto *surface=SDL_CreateRGBSurfaceWithFormatFrom(frame.data.data(),1920,1080,24,5760,SDL_PIXELFORMAT_RGB24);if(!surface||SDL_SaveBMP(surface,argv[3])<0)return 4;SDL_FreeSurface(surface);
    std::cout<<std::fixed<<std::setprecision(4)<<"{\"raw_yuyv_ms\":"<<yuyv<<",\"raw_nv12_ms\":"<<nv12<<",\"jpeg_decode_ms\":"<<decode<<",\"jpeg_mse\":"<<error<<"}\n";
}
