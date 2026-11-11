#pragma once
#include "core.hpp"
#include <stdexcept>
#ifdef _WIN32
#include "audio_windows_buffer.hpp"
#endif
class Audio {
    SDL_AudioDeviceID input=0,output=0;
    static void capture(void *ctx,Uint8 *bytes,int len) { auto &a=*static_cast<Audio*>(ctx); a.ring.push(reinterpret_cast<int16_t*>(bytes),static_cast<size_t>(len)/2); }
    static void playback(void *ctx,Uint8 *bytes,int len) {
        auto &a=*static_cast<Audio*>(ctx); auto *dst=reinterpret_cast<int16_t*>(bytes); size_t n=static_cast<size_t>(len)/2;
        float gain=a.muted?0:a.volume.load();
        a.ring.pull(dst,n,gain);
    }
public:
#ifdef _WIN32
    WindowsAudioBuffer ring;
#else
    AudioRing ring;
#endif
    std::atomic<float> volume{0.7f};
    std::atomic<bool> muted{false};
    std::string mode;
    explicit Audio(int latency=20):ring(static_cast<size_t>(latency)*96) {}
    ~Audio() { stop(); }
    bool ownsDevice(Uint32 id)const{return id==input||id==output;}
    void start(const char *in,const char *out) {
        stop(); ring.clear();
        SDL_AudioSpec want{},have{}; want.freq=48000; want.format=AUDIO_S16SYS; want.channels=2; want.samples=128; want.callback=playback; want.userdata=this;
#ifdef _WIN32
        want.samples=256;
#endif
        output=SDL_OpenAudioDevice(out,0,&want,&have,0);
        if(!output) throw std::runtime_error("Cannot open audio output: "+std::string(SDL_GetError()));
        mode="48 kHz stereo / output period "+std::to_string(have.samples)+" samples / "+SDL_GetCurrentAudioDriver();
#ifdef _WIN32
        const auto outputPeriod=have.samples;
        ring.configure(outputPeriod,outputPeriod);
#endif
        {
            want.callback=capture;
            input=SDL_OpenAudioDevice(in,1,&want,&have,0);
            if(!input) { std::string error=SDL_GetError(); stop(); throw std::runtime_error("Cannot open XR1 Lite audio input: "+error+". Select the XR1 Lite / USB Digital Audio input; close other capture apps."); }
            mode+=" / input period "+std::to_string(have.samples)+" samples";
#ifdef _WIN32
            ring.configure(outputPeriod,have.samples);
#endif
            SDL_PauseAudioDevice(input,0);
        }
        SDL_PauseAudioDevice(output,0);
    }
    void stop() { if(input) {SDL_CloseAudioDevice(input);input=0;} if(output) {SDL_CloseAudioDevice(output);output=0;} }
};
