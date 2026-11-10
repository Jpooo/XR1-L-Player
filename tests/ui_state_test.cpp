#include "../src/core.hpp"
#include "../src/frame_cadence.hpp"
#include "../src/ui.hpp"
#include <cassert>
#include <iostream>
int main(){
    SharedVideo stream;Frame frame;double now=monotime();
    for(int i=0;i<=60;++i){frame.received=now-1+i/60.0;stream.publish(frame);}
    assert(std::abs(stream.captureFps()-60)<.001);
    stream.captured=0;assert(stream.captureFps()==0);
    FrameRate rate;for(int i=0;i<=300;++i)rate.record(i/60.0);assert(std::abs(rate.fps(5.01)-60)<.001);
    assert(rate.fps(6.1)==0);rate.reset();assert(rate.fps(0)==0);
    // Layout hit areas stay in the logical viewport, including the smallest
    // supported window (SDL handles aspect-preserving coordinate conversion).
    auto inside=[](ui::Box box){assert(box.x>=0&&box.y>=0&&box.x+box.w<=1100&&box.y+box.h<=ui::height);};
    for(auto box:ui::navigation)inside(box);
    for(auto box:ui::settingControls)inside(box);
    for(auto box:ui::preferenceControls)inside(box);
    std::cout<<"PASS capture/frame cadence, restart reset and menu hit areas\n";
}
