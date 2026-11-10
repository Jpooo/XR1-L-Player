#pragma once
#include <algorithm>
#include <cmath>
namespace ui {
inline double menuFrameRate(int displayHz,bool reducedMotion,bool playing){
    if(reducedMotion)return 20;
    if(playing)return 60; // Do not add high-refresh menu work to live capture.
    if(displayHz<=0)return 60;
    if(displayHz==59||displayHz==119||displayHz==239)++displayHz; // SDL2 reports whole Hz for fractional modes.
    return std::clamp(displayHz,24,240);
}
// Keep the deadline on a fixed phase instead of accumulating timer oversleep.
// A late paint skips expired deadlines; it never queues catch-up frames.
class MenuFrameClock {
    double interval=1.0/60,next=0;
    bool scheduled=false;
public:
    void rate(double hz){
        double value=1/std::clamp(std::isfinite(hz)&&hz>0?hz:60.0,1.0,240.0);
        if(std::abs(value-interval)>1e-9){interval=value;scheduled=false;}
    }
    bool due(double now)const{return !scheduled||now>=next;}
    double deadline(double now)const{return scheduled?next:now;}
    void painted(double now){
        if(!scheduled){next=now;scheduled=true;}
        if(now>=next){next+=(std::floor((now-next)/interval)+1)*interval;if(next<=now)next+=interval;}
    }
};
// A quiet hover suits the rectangular capture box; Motion Off is level and still.
inline float deviceBob(double seconds,bool reducedMotion){return reducedMotion?0.0f:float(std::sin(seconds*1.05)*2);}
inline double deviceTilt(double seconds,bool reducedMotion){return reducedMotion?0.0:std::sin(seconds*.7)*.8;}
}
