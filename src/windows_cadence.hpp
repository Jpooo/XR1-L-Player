#pragma once
#include <array>
#include <cstddef>
// Measure intervals between actual frame events. Sampling integer counters at
// arbitrary UI boundaries can make capture look one frame ahead of display.
class WindowsFrameRate {
    std::array<double,121> times{};
    std::size_t next=0,count=0;
public:
    void reset(){next=count=0;}
    void record(double now){
        if(count&&now<=times[(next+times.size()-1)%times.size()])return;
        times[next]=now;next=(next+1)%times.size();if(count<times.size())++count;
    }
    double fps(double now)const{
        if(count<2)return 0;
        double last=times[(next+times.size()-1)%times.size()];
        double first=times[(next+times.size()-count)%times.size()];
        if(now-last>1||last<=first)return 0;
        return (count-1)/(last-first);
    }
};
