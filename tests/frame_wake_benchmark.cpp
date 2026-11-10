// Controlled ready-frame -> UI ownership timing, not controller/HDMI latency.
#ifndef BENCH_CORE_HEADER
#define BENCH_CORE_HEADER "../src/core.hpp"
#define USE_FRAME_CONDITION
#endif
#include BENCH_CORE_HEADER
#include <thread>
#include <future>
#include <cassert>
#include <iostream>
#include <iomanip>
int main(){
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0);
    auto *window=SDL_CreateWindow("Ready frame wake comparison",0,0,100,100,SDL_WINDOW_HIDDEN);assert(window);
    SharedVideo shared;
#ifdef USE_FRAME_CONDITION
    Frame pending;pending.received=monotime();shared.publish(pending);
    assert(shared.waitForFrame(100));assert(shared.take(pending));assert(!shared.waitForFrame(1));
    auto waiting=std::async(std::launch::async,[&](){return shared.waitForFrame(500);});
    shared.fail("disconnect");assert(waiting.get());{std::lock_guard<std::mutex> lock(shared.mutex);shared.error.clear();}
#else
    shared.wakeEvent=SDL_RegisterEvents(1);
#endif
    constexpr int count=180;
    std::atomic<bool> consumed{true};std::vector<double> times;
    std::thread capture([&](){Frame frame;auto deadline=std::chrono::steady_clock::now();for(int i=0;i<count;++i){
        while(!consumed.load())std::this_thread::yield();
        deadline+=std::chrono::microseconds(16667);std::this_thread::sleep_until(deadline);
        frame.received=monotime();consumed=false;shared.publish(frame);
    }});
    Frame frame;
    for(int i=0;i<count;){if(shared.take(frame)){times.push_back((monotime()-frame.received)*1000);consumed=true;++i;}
#ifdef USE_FRAME_CONDITION
        else shared.waitForFrame(8);
#else
        else {SDL_Event event;SDL_WaitEventTimeout(&event,100);}
#endif
    }
    capture.join();std::sort(times.begin(),times.end());double sum=0;for(auto t:times)sum+=t;
    std::cout<<std::fixed<<std::setprecision(4)<<"{\"mean_ms\":"<<sum/times.size()<<",\"p50_ms\":"<<times[times.size()/2]<<",\"p95_ms\":"<<times[size_t(times.size()*.95)]<<",\"max_ms\":"<<times.back()<<"}\n";
    SDL_DestroyWindow(window);SDL_Quit();
}
