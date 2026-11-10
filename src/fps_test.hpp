#pragma once
#include "core.hpp"
#include <sstream>
#include <iomanip>
struct FpsCounters {
    uint64_t received=0,decoded=0,shown=0,skipped=0,pendingDropped=0,invalid=0,late=0,audioDropped=0,audioStarved=0;
};
class FpsTest {
    FpsCounters baseline{},latest{};
    double began=0,ended=0,uploadTotal=0,uploadMax=0,presentTotal=0,presentMax=0;
    uint64_t uploads=0,presents=0,slowUploads=0,slowPresents=0;
public:
    bool active=false,complete=false,testPicture=false,visible=true;
    void start(double now,const FpsCounters &counts,bool demo=false){uploadTotal=uploadMax=presentTotal=presentMax=0;uploads=presents=slowUploads=slowPresents=0;baseline=latest=counts;began=now;ended=now;active=true;complete=false;testPicture=demo;visible=true;}
    void update(double now,const FpsCounters &counts){if(!active)return;latest=counts;ended=now;if(now-began>=30){active=false;complete=true;}}
    void finish(double now,const FpsCounters &counts){if(!active)return;update(now,counts);active=false;complete=true;}
    void close(double now,const FpsCounters &counts){if(active)finish(now,counts);visible=false;}
    void recordUpload(double ms){if(active){uploadTotal+=ms;uploadMax=std::max(uploadMax,ms);++uploads;if(ms>8)++slowUploads;}}
    void recordPresent(double ms){if(active){presentTotal+=ms;presentMax=std::max(presentMax,ms);++presents;if(ms>8)++slowPresents;}}
    double elapsed()const{return std::max(0.0,ended-began);}
    double receivedFps()const{return elapsed()>0?(latest.received-baseline.received)/elapsed():0;}
    double shownFps()const{return elapsed()>0?(latest.shown-baseline.shown)/elapsed():0;}
    std::string summary()const{
        std::ostringstream s;s<<std::fixed<<std::setprecision(1);
        if(active)s<<"FPS test: "<<std::min(30,int(elapsed()))<<" / 30 s";
        else s<<"FPS result";
        s<<"  |  Capture "<<receivedFps()<<"  /  Shown "<<shownFps();
        if(testPicture)s<<"  (test picture)";
        return s.str();
    }
    std::string report()const{
        std::ostringstream s;s<<std::fixed<<std::setprecision(2);
        s<<
#ifdef _WIN32
        "XR1-L Player 1.5.7 Windows FPS test\nSource: "
#else
        "XR1-L Player 1.5.7 Linux FPS test\nSource: "
#endif
         <<(testPicture?"generated test picture (not a hardware test)":"native capture card")
         <<"\nTest status: "<<(elapsed()>=30?"completed":"stopped early")<<"\nDuration: "<<elapsed()<<" seconds"
#ifdef _WIN32
         <<"\nCapture arrivals: "
#else
         <<"\nValid capture frames: "
#endif
         <<latest.received-baseline.received<<" / "<<receivedFps()<<" fps"
         <<"\nDecoded frames: "<<latest.decoded-baseline.decoded<<"\nFrames submitted for display: "<<latest.shown-baseline.shown<<" / "<<shownFps()<<" fps"
#ifdef _WIN32
         <<"\nCapture arrivals more than 25 ms apart: "<<latest.late-baseline.late
         <<"\nReplaced pending decode samples: "<<latest.pendingDropped-baseline.pendingDropped
         <<"\nReplaced pending display frames: "<<latest.skipped-baseline.skipped
#else
         <<"\nSkipped/stale frames (driver or preview): "<<latest.skipped-baseline.skipped
#endif
         <<"\nInvalid frames: "<<latest.invalid-baseline.invalid
         <<"\nAudio dropped samples: "<<latest.audioDropped-baseline.audioDropped<<"\nAudio starved samples: "<<latest.audioStarved-baseline.audioStarved
         <<"\nTexture upload call mean/max: "<<(uploads?uploadTotal/uploads:0)<<" / "<<uploadMax<<" ms"
         <<"\nPresent call mean/max: "<<(presents?presentTotal/presents:0)<<" / "<<presentMax<<" ms"
         <<"\nUploads/presents over 8 ms: "<<slowUploads<<" / "<<slowPresents
         <<"\nTarget: native 60 or 59.94 fps. Counts measure received samples and renderer submissions, not HDMI content uniqueness, display scanout or controller latency.\n";
        return s.str();
    }
};
