#pragma once
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#ifndef _WIN32
#include "frame_cadence.hpp"
#endif
inline double monotime() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline std::string lower(std::string s) { for (auto &c:s) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }
inline bool xrname(const std::string &s) { auto l=lower(s); return l.find("xr1")!=std::string::npos || l.find("evga")!=std::string::npos; }
// A bounded stereo PCM queue. The producer always keeps the newest samples.
class AudioRing {
    std::mutex mutex;
    std::vector<int16_t> data;
    size_t head=0, count=0;
public:
    std::atomic<uint64_t> dropped{0}, starved{0};
    explicit AudioRing(size_t samples=1920):data(samples) {}
    void push(const int16_t *src, size_t n) {
        std::lock_guard<std::mutex> lock(mutex);
        if (n>data.size()) { dropped+=n-data.size(); src+=n-data.size(); n=data.size(); }
        if (count+n>data.size()) { size_t skip=count+n-data.size(); head=(head+skip)%data.size(); count-=skip; dropped+=skip; }
        for(size_t i=0;i<n;++i) data[(head+count+i)%data.size()]=src[i];
        count+=n;
    }
    void pull(int16_t *dst,size_t n,float gain) {
        std::lock_guard<std::mutex> lock(mutex);
        size_t take=std::min(n,count);
        for(size_t i=0;i<take;++i) dst[i]=static_cast<int16_t>(std::clamp(data[(head+i)%data.size()]*gain,-32768.0f,32767.0f));
        std::fill(dst+take,dst+n,0);
        head=(head+take)%data.size(); count-=take;
        starved+=n-take;
    }
    size_t queued() { std::lock_guard<std::mutex> lock(mutex); return count; }
    void clear() { std::lock_guard<std::mutex> lock(mutex); count=head=0; dropped=0; starved=0; }
};
struct Frame {
    std::vector<uint8_t> data;
    Uint32 pixel=SDL_PIXELFORMAT_YUY2;
    int pitch=3840;
    double received=0;
};
struct SharedVideo {
    std::mutex mutex;
    std::condition_variable ready;
    Frame frame;
    bool dirty=false;
    std::string error, mode;
    std::atomic<uint64_t> captured{0}, skipped{0}, corrupt{0};
    // Assigned before starting capture. Only one notification is needed while
    // a frame is pending; replacing it must not build a queue of SDL events.
#ifndef _WIN32
    FrameRate captureCadence;
    double captureFps(){std::lock_guard<std::mutex> lock(mutex);return captured.load()?captureCadence.fps(monotime()):0;}
#endif
    Uint32 wakeEvent=0;
    void wake() { if(wakeEvent) {SDL_Event e{};e.type=wakeEvent;SDL_PushEvent(&e);} }
    void publish(Frame &f) {
        bool notify=false;
        {
            std::lock_guard<std::mutex> lock(mutex);
#ifndef _WIN32
            if(captured==0)captureCadence.reset();
            captureCadence.record(f.received);
#endif
            notify=!dirty;
            if(dirty)++skipped;
            std::swap(frame,f);dirty=true;++captured;
        }
        ready.notify_one();
        if(notify)wake();
    }
    bool take(Frame &f) {
        std::lock_guard<std::mutex> lock(mutex);
        if(!dirty)return false;
        std::swap(frame,f);dirty=false;return true;
    }
    bool waitForFrame(int milliseconds){
        std::unique_lock<std::mutex> lock(mutex);
        return ready.wait_for(lock,std::chrono::milliseconds(milliseconds),[&](){return dirty||!error.empty();});
    }
    void fail(const std::string &s) {
        {std::lock_guard<std::mutex> lock(mutex);error=s;}
        ready.notify_one();wake();
    }
};
