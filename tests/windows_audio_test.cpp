#include "../src/audio_windows_buffer.hpp"
#include "../src/windows_fps.hpp"
#include <cassert>
#include <iostream>
static void clockTest(double ppm){
    WindowsAudioBuffer ring;ring.configure(256,256);
    std::vector<int16_t> input(960),output(512);double nextInput=0,nextOutput=0;uint64_t generated=0;int packets=0;int maximumEdge=0;int16_t last=0;
    while(nextOutput<120){
        if(nextInput<=nextOutput){
            for(size_t i=0;i<480;++i){int16_t value=static_cast<int16_t>(12000*std::sin(2*3.141592653589793*440*generated++/48000));input[2*i]=input[2*i+1]=value;}
            ring.push(input.data(),input.size());
            // Deliberately burst input 0..3 ms early/late, at a different clock.
            ++packets;nextInput=packets*.01/(1+ppm/1e6)+.0015*std::sin(packets*1.7);
        }else{
            ring.pull(output.data(),output.size(),1);
            for(size_t i=0;i<256;++i){assert(output[2*i]==output[2*i+1]);maximumEdge=std::max(maximumEdge,std::abs(int(output[2*i])-last));last=output[2*i];}
            assert(ring.queued()<=3072);nextOutput+=256.0/48000;
        }
    }
    std::cout<<"Clock "<<ppm<<" ppm: dropped "<<ring.dropped<<", starved "<<ring.starved<<", maximum waveform edge "<<maximumEdge<<"\n";
    assert(ring.dropped==0);assert(ring.starved==0);assert(maximumEdge<1600);
}
int main(){
    for(double ppm:{-300.0,0.0,300.0})clockTest(ppm);
    WindowsAudioBuffer ring;std::vector<int16_t> input(3072,14000),out(512);
    ring.push(input.data(),input.size());ring.pull(out.data(),out.size(),1);
    int previous=out.back(),maxEdge=0;
    for(int k=0;k<10;++k){ring.pull(out.data(),out.size(),1);for(size_t i=0;i<out.size();i+=2){maxEdge=std::max(maxEdge,std::abs(int(out[i])-previous));previous=out[i];}}
    assert(ring.starved>0);assert(out.back()==0);assert(maxEdge<200);
    ring.push(input.data(),input.size());ring.pull(out.data(),out.size(),1);assert(out.back()>13000);
    ring.pull(out.data(),out.size(),0);assert(out.back()==0);
    ring.push(input.data(),input.size());ring.push(input.data(),input.size());assert(ring.dropped>0&&ring.queued()==3072);
    ring.clear();assert(!ring.queued()&&!ring.starved&&!ring.dropped);
    WindowsFpsTest fps;fps.start(10,{});fps.update(40,{1800,1800,1800});assert(fps.complete&&!fps.active&&fps.receivedFps()==60&&fps.shownFps()==60);
    fps.start(50,{100,100,100});fps.update(80,{1000,1000,1000});assert(fps.receivedFps()==30&&fps.shownFps()==30);
    fps.start(90,{},true);fps.finish(95,{300,300,280});assert(fps.elapsed()==5&&fps.testPicture&&fps.report().find("not a hardware test")!=std::string::npos);
    std::cout<<"PASS Windows jitter/clock drift, stereo continuity, underrun recovery, gain ramp, overflow bounds and FPS counters\n";
}
