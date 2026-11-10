#include "../src/windows_presenter.hpp"
#include "../src/windows_fps.hpp"
#include "../src/windows_cadence.hpp"
#include <cassert>
#include <iostream>
int main(){
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0);
    SDL_Window *window=SDL_CreateWindow("Windows video upload test",0,0,320,180,SDL_WINDOW_HIDDEN);assert(window);
    SDL_Renderer *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);assert(renderer);
    {
        WindowsVideoUpload uploader;
        for(Uint32 pixel:{SDL_PIXELFORMAT_NV12,SDL_PIXELFORMAT_IYUV,SDL_PIXELFORMAT_YV12,SDL_PIXELFORMAT_YUY2,SDL_PIXELFORMAT_UYVY,SDL_PIXELFORMAT_RGB24}){
            uploader.reset();Frame frame;frame.pixel=pixel;
            bool planar=pixel==SDL_PIXELFORMAT_NV12||pixel==SDL_PIXELFORMAT_IYUV||pixel==SDL_PIXELFORMAT_YV12;
            frame.pitch=planar?1920:pixel==SDL_PIXELFORMAT_RGB24?5760:3840;
            frame.data.resize(static_cast<size_t>(frame.pitch)*(planar?1620:1080));
            SDL_Texture *first=nullptr,*second=nullptr;
            for(int i=0;i<4;++i){
                std::fill(frame.data.begin(),frame.data.end(),128);
                // Alternate complete black/white frames. Readback must show the
                // newest frame, including when cycling back to a reused texture.
                const uint8_t level=i%2?235:16;
                if(pixel==SDL_PIXELFORMAT_RGB24)std::fill(frame.data.begin(),frame.data.end(),i%2?255:0);
                else if(planar)std::fill_n(frame.data.begin(),1920*1080,level);
                else for(size_t j=pixel==SDL_PIXELFORMAT_UYVY?1:0;j<frame.data.size();j+=2)frame.data[j]=level;
                auto *texture=uploader.upload(renderer,frame);assert(texture&&uploader.destinations()==2);
                if(i==0)first=texture;
                if(i==1){second=texture;assert(first!=second);}
                if(i>=2)assert(texture==(i%2?second:first));
                assert(SDL_RenderCopy(renderer,texture,nullptr,nullptr)==0);
                uint8_t color[4];SDL_Rect center{160,90,1,1};assert(SDL_RenderReadPixels(renderer,&center,SDL_PIXELFORMAT_RGBA32,color,4)==0);
                for(int c=0;c<3;++c)assert(i%2?color[c]>240:color[c]<15);
                SDL_RenderPresent(renderer);
            }
            std::cout<<"PASS newest-frame readback with reused upload destinations: "<<SDL_GetPixelFormatName(pixel)<<"\n";
        }
        uploader.reset();assert(!uploader.destinations());
        WindowsPlaybackScheduling scheduling;scheduling.start();scheduling.stop();scheduling.stop();
        WindowsFrameRate cadence;
        for(int i=0;i<=300;++i)cadence.record(i/60.0);
        assert(std::abs(cadence.fps(5.01)-60)<.001); // UI polls after the frame event.
        assert(cadence.fps(6.1)==0);
        cadence.reset();assert(cadence.fps(0)==0);
        for(int i=0;i<=290;++i)cadence.record(i/58.0);
        assert(std::abs(cadence.fps(5.01)-58)<.001);
        WindowsFpsTest test;assert(!test.visible&&!test.active);test.start(1,{});test.recordUpload(2);test.recordUpload(10);test.recordPresent(1);test.recordPresent(9);test.update(31,{1800,1800,1740});
        auto report=test.report();assert(report.find("Texture upload call mean/max: 6.00 / 10.00 ms")!=std::string::npos);
        assert(report.find("Uploads/presents over 8 ms: 1 / 1")!=std::string::npos);
        assert(test.receivedFps()==60&&test.shownFps()==58); // No rounding a real loss up to 60.
        test.start(40,{});test.finish(41,{60,60,60});assert(test.report().find("Texture upload call mean/max: 0.00 / 0.00 ms")!=std::string::npos);
        std::cout<<"PASS reset/recreate, scheduling cleanup and measured 60 capture / 58 shown diagnostics\n";
    }
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
}
