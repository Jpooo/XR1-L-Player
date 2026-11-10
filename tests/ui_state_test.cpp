#include "../src/fps_test.hpp"
#include "../src/frame_cadence.hpp"
#include <cassert>
#include <iostream>
int main(){
    SharedVideo stream;Frame frame;double now=monotime();
    for(int i=0;i<=60;++i){frame.received=now-1+i/60.0;stream.publish(frame);}
    assert(std::abs(stream.captureFps()-60)<.001);
    stream.captured=0;assert(stream.captureFps()==0); // A restarted feed has no received frames yet.
    FpsTest test;assert(!test.visible&&!test.active&&!test.complete);test.start(0,{});assert(test.visible&&test.active);test.update(30,{1800,1800,1740});
    assert(test.complete&&test.receivedFps()==60&&test.shownFps()==58);
    test.close(31,{1800,1800,1740});assert(!test.visible&&!test.active&&test.complete);
    test.start(40,{100,100,100});test.close(45,{400,400,400});
    assert(!test.visible&&!test.active&&test.complete&&test.elapsed()==5);
    assert(test.report().find("stopped early")!=std::string::npos);
    test.start(50,{},true);test.recordUpload(12);test.recordPresent(9);test.finish(52,{120,120,120});
    assert(test.visible&&test.testPicture&&test.report().find("1 / 1")!=std::string::npos);
    FrameRate rate;for(int i=0;i<=300;++i)rate.record(i/60.0);assert(std::abs(rate.fps(5.01)-60)<.001);
    assert(rate.fps(6.1)==0);rate.reset();assert(rate.fps(0)==0);
    std::cout<<"PASS FPS completion/close/restart, partial reports, actual 60-versus-58 counts and frame interval rates\n";
}
