#pragma once
#include "core.hpp"
// WASAPI input and output have independent clocks and burst sizes. Keep a
// small priming reserve and gently track clock drift rather than periodically
// dropping whole packets or inserting hard zero edges into the waveform.
class WindowsAudioBuffer {
    std::mutex mutex;
    std::vector<int16_t> data;
    size_t head=0,count=0,target=0;
    double fraction=0,errorAverage=0;
    float gainNow=0,last[2]{},anchor[2]{};
    unsigned crossfade=0,fadeOut=0;
    bool primed=false,jump=false;
    static constexpr unsigned ramp=96; // 2 ms at 48 kHz
    size_t capacity()const{return data.size()/2;}
public:
    std::atomic<uint64_t> dropped{0},starved{0};
    explicit WindowsAudioBuffer(size_t samples=3072):data(std::max<size_t>(samples/2,8)*2){configure(256,256);}
    void configure(size_t outputFrames,size_t inputFrames){
        std::lock_guard<std::mutex> lock(mutex);
        target=std::min(capacity()-2,std::max<size_t>(640,2*std::max(outputFrames,inputFrames)));
    }
    void push(const int16_t *src,size_t samples){
        std::lock_guard<std::mutex> lock(mutex);
        size_t n=samples/2;
        if(n>capacity()){dropped+=2*(n-capacity());src+=2*(n-capacity());n=capacity();}
        if(count+n>capacity()){
            size_t skip=count+n-capacity();head=(head+skip)%capacity();count-=skip;dropped+=2*skip;
            fraction=0;jump=true;
        }
        for(size_t i=0;i<n;++i)for(size_t c=0;c<2;++c)data[2*((head+count+i)%capacity())+c]=src[2*i+c];
        count+=n;
    }
    void pull(int16_t *dst,size_t samples,float gain){
        std::lock_guard<std::mutex> lock(mutex);
        const size_t frames=samples/2;
        if(!primed&&count>=target){primed=true;fraction=0;errorAverage=0;jump=true;}
        if(jump){anchor[0]=last[0];anchor[1]=last[1];crossfade=ramp;jump=false;}
        errorAverage+=(static_cast<double>(count)-target-errorAverage)*std::min(1.0,frames/24000.0);
        double step=1.0+std::clamp(errorAverage/240000.0,-0.003,0.003);
        for(size_t i=0;i<frames;++i){
            if(primed&&count<2){primed=false;fadeOut=ramp;anchor[0]=last[0];anchor[1]=last[1];starved+=2*(frames-i);}
            if(primed){
                gainNow+=std::clamp(gain-gainNow,-1.0f/ramp,1.0f/ramp);
                float blend=crossfade?1.0f-float(crossfade-1)/ramp:1.0f;
                for(size_t c=0;c<2;++c){
                    float a=data[2*head+c],b=data[2*((head+1)%capacity())+c];
                    float value=(a+float(fraction)*(b-a))*gainNow;
                    last[c]=crossfade?anchor[c]*(1-blend)+value*blend:value;
                }
                if(crossfade)--crossfade;
                fraction+=step;size_t used=static_cast<size_t>(fraction);used=std::min(used,count);fraction-=used;
                head=(head+used)%capacity();count-=used;
            }else{
                for(size_t c=0;c<2;++c)last[c]=fadeOut?anchor[c]*float(fadeOut-1)/ramp:0;
                if(fadeOut)--fadeOut;
                gainNow=0;
            }
            for(size_t c=0;c<2;++c)dst[2*i+c]=static_cast<int16_t>(std::clamp(last[c],-32768.0f,32767.0f));
        }
        if(samples%2)dst[samples-1]=0;
    }
    size_t queued(){std::lock_guard<std::mutex> lock(mutex);return count*2;}
    void clear(){std::lock_guard<std::mutex> lock(mutex);head=count=0;fraction=errorAverage=0;gainNow=0;last[0]=last[1]=anchor[0]=anchor[1]=0;crossfade=fadeOut=0;primed=jump=false;dropped=0;starved=0;}
};
