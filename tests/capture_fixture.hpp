#pragma once
// Hardware substitutes for regression executables only. This header is never
// included by the shipping app, so it has no test picture, tone or test switch.
#include "../src/video.hpp"
#include <thread>
#include <cassert>
namespace fixture {
inline bool enabled=false;
inline constexpr SDL_AudioDeviceID inputId=0xfeed1234;
class PcmInput {
    std::atomic<bool> running{false},paused{true};std::thread worker;
public:
    ~PcmInput(){stop();}
    void stop(){running=false;if(worker.joinable())worker.join();}
    void pause(bool value){paused=value;}
    void start(SDL_AudioSpec spec){
        stop();paused=true;running=true;
        worker=std::thread([this,spec](){
            std::vector<int16_t> samples(size_t(spec.samples)*spec.channels);double phase=0;
            auto next=std::chrono::steady_clock::now();
            while(running){
                if(paused){std::this_thread::sleep_for(std::chrono::milliseconds(1));next=std::chrono::steady_clock::now();continue;}
                for(size_t i=0;i<samples.size();i+=spec.channels){auto value=int16_t(std::sin(phase)*1800);for(int c=0;c<spec.channels;++c)samples[i+c]=value;phase+=2*3.141592653589793*440/spec.freq;}
                spec.callback(spec.userdata,reinterpret_cast<Uint8*>(samples.data()),int(samples.size()*2));
                next+=std::chrono::nanoseconds(int64_t(1e9*spec.samples/spec.freq));std::this_thread::sleep_until(next);
            }
        });
    }
};
inline PcmInput pcm;
inline SDL_AudioDeviceID openAudio(const char *name,int input,const SDL_AudioSpec *want,SDL_AudioSpec *have,int changes){
    if(!input)return SDL_OpenAudioDevice(name,input,want,have,changes);
    *have=*want;pcm.start(*want);return inputId;
}
inline void pauseAudio(SDL_AudioDeviceID id,int paused){if(id==inputId)pcm.pause(paused!=0);else SDL_PauseAudioDevice(id,paused);}
inline void closeAudio(SDL_AudioDeviceID id){if(id==inputId)pcm.stop();else SDL_CloseAudioDevice(id);}
inline int audioCount(int input){return input?1:SDL_GetNumAudioDevices(input);}
inline const char *audioName(int index,int input){return input?"XR1 Lite fixture HDMI":SDL_GetAudioDeviceName(index,input);}
inline std::vector<VideoDevice> devices(){return enabled?std::vector<VideoDevice>{{"fixture://xr1-lite","XR1 Lite UI fixture","3842:310a"}}:std::vector<VideoDevice>{};}
class Capture {
    SharedVideo &shared;std::atomic<bool> running{false};std::thread worker;double began=0;
public:
    explicit Capture(SharedVideo &value):shared(value){}
    ~Capture(){stop();}
    void stop(){running=false;if(worker.joinable())worker.join();}
    void start(const std::string &,const std::string &,unsigned){
        stop();assert(enabled);began=monotime();
        {std::lock_guard<std::mutex> lock(shared.mutex);shared.error.clear();shared.dirty=false;shared.captured=0;shared.skipped=0;shared.corrupt=0;shared.mode="UI fixture 1080p60";}
        running=true;worker=std::thread([this](){
            const uint8_t colors[8][3]={{220,230,235},{220,200,55},{60,200,195},{65,185,95},{190,70,190},{205,65,65},{65,100,210},{18,24,35}};
            std::vector<uint8_t> base(size_t(1920)*1080*3);
            for(int y=0;y<1080;++y)for(int x=0;x<1920;++x)for(int c=0;c<3;++c)base[size_t(y*1920+x)*3+c]=uint8_t(colors[x/240][c]*(y>820?.3:1));
            Frame frame;auto next=std::chrono::steady_clock::now();
            while(running){
                frame.pixel=SDL_PIXELFORMAT_RGB24;frame.pitch=5760;frame.data=base;
                int marker=int(std::fmod((monotime()-began)*480,1920));
                for(int y=0;y<1080;++y)for(int x=std::max(0,marker-12);x<std::min(1920,marker+12);++x)for(int c=0;c<3;++c)frame.data[size_t(y*1920+x)*3+c]=255;
                frame.received=monotime();shared.publish(frame);next+=std::chrono::microseconds(16667);std::this_thread::sleep_until(next);
            }
        });
    }
#ifdef _WIN32
    WinCaptureSnapshot snapshot(){WinCaptureSnapshot value;value.samples=shared.captured.load();double elapsed=monotime()-began;value.fps=elapsed>0?value.samples/elapsed:0;return value;}
#endif
};
}
// Keep the real audio bridge and callback; substitute only its input device.
#define SDL_OpenAudioDevice fixture::openAudio
#define SDL_PauseAudioDevice fixture::pauseAudio
#define SDL_CloseAudioDevice fixture::closeAudio
#include "../src/audio.hpp"
#undef SDL_OpenAudioDevice
#undef SDL_PauseAudioDevice
#undef SDL_CloseAudioDevice
using FixtureVideoCapture=fixture::Capture;
#define Capture FixtureVideoCapture
#define videoDevices fixture::devices
#define SDL_GetNumAudioDevices fixture::audioCount
#define SDL_GetAudioDeviceName fixture::audioName
