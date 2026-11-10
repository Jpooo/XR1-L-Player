#include "../src/core.hpp"
#include <cassert>
#include <future>
#include <thread>
#include <iostream>
#include <iomanip>
static void drain() {SDL_Event e;while(SDL_PollEvent(&e)) {}}
static int count(Uint32 type){int n=0;SDL_Event e;while(SDL_PollEvent(&e))if(e.type==type)++n;return n;}
static double benchmark(bool eventDriven,Uint32 eventType) {
    SharedVideo shared;shared.wakeEvent=eventDriven?eventType:0;
    constexpr int frames=240;
    std::atomic<bool> taken{true};
    std::thread producer([&]() {
        Frame next;auto deadline=std::chrono::steady_clock::now();
        for(int i=0;i<frames;++i) {
            while(!taken.load())std::this_thread::yield();
            deadline+=std::chrono::microseconds(4167);
            std::this_thread::sleep_until(deadline);
            next.received=monotime();taken=false;shared.publish(next);
        }
    });
    Frame f;double total=0;
    for(int i=0;i<frames;) {
        if(shared.take(f)){total+=(monotime()-f.received)*1000;++i;taken=true;}
        else if(eventDriven){SDL_Event e;SDL_WaitEventTimeout(&e,100);}
        else SDL_Delay(1);
    }
    producer.join();drain();return total/frames;
}
int main() {
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0);
    auto *window=SDL_CreateWindow("Frame handoff test",0,0,100,100,SDL_WINDOW_HIDDEN);assert(window);
    Uint32 type=SDL_RegisterEvents(1);assert(type!=static_cast<Uint32>(-1));drain();
    SharedVideo shared;shared.wakeEvent=type;Frame producer;
    producer.data={1};shared.publish(producer);producer.data={2};shared.publish(producer);producer.data={3};shared.publish(producer);
    assert(count(type)==1);assert(shared.skipped==2);
    Frame uploading;assert(shared.take(uploading)&&uploading.data[0]==3);
    // Simulate a renderer that stalls while owning the previous frame.
    auto future=std::async(std::launch::async,[&]() {producer.data={4};shared.publish(producer);producer.data={5};shared.publish(producer);});
    assert(future.wait_for(std::chrono::milliseconds(100))==std::future_status::ready);future.get();
    assert(uploading.data[0]==3);assert(count(type)==1);
    assert(shared.take(uploading)&&uploading.data[0]==5);assert(!shared.take(uploading));
    shared.fail("disconnect");assert(count(type)==1);assert(shared.error=="disconnect");
    std::cout<<"PASS coalesced wakeups, latest frame ownership, capture continues during renderer stalls, error wakeup\n";
    auto polling=benchmark(false,type),events=benchmark(true,type);
    std::cout<<std::fixed<<std::setprecision(3)<<"Synthetic CPU handoff mean: 1 ms polling = "<<polling<<" ms, event wake = "<<events<<" ms (not end-to-end latency)\n";
    SDL_DestroyWindow(window);SDL_Quit();
}
