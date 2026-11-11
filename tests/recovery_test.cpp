#include "../src/connection_recovery.hpp"
#include <cassert>
#include <iostream>
int main(){
    using namespace connection;
    Target target{{"/dev/video0","EVGA XR1 Lite","3842:310a"},"HDMI input","Headphones","nv12"};
    std::vector<VideoDevice> devices={{"/dev/video8","Webcam","1234:5678"},{"/dev/video2","EVGA XR1 Lite","3842:310a"}};
    auto choice=resolve(target,devices,{"Other microphone","HDMI input"},{"Speakers","Headphones"});
    assert(choice.state==Match::Ready&&choice.video==1&&choice.input==1&&choice.output==1);
    assert(resolve(target,{}, {"HDMI input"},{"Headphones"}).state==Match::CardMissing);
    assert(resolve(target,devices,{"Other microphone"},{"Headphones"}).state==Match::InputMissing);
    assert(resolve(target,devices,{"HDMI input"},{"Speakers"}).state==Match::OutputMissing);
    devices.push_back({"/dev/video3","EVGA XR1 Lite","3842:310a"});assert(resolve(target,devices,{"HDMI input"},{"Headphones"}).state==Match::Ambiguous);
    target.video.path="/dev/v4l/by-id/usb-EVGA-unit-a";assert(resolve(target,devices,{"HDMI input"},{"Headphones"}).state==Match::CardMissing);
    devices.push_back({target.video.path,"Other camera","1234:5678"});assert(resolve(target,devices,{"HDMI input"},{"Headphones"}).state==Match::CardMissing);
    devices.back()=target.video;assert(resolve(target,devices,{"HDMI input","HDMI input"},{"Headphones"}).state==Match::Ambiguous);
    assert(resolve(target,devices,{"HDMI input"},{"Headphones","Headphones"}).state==Match::Ambiguous);
    target.output.clear();choice=resolve(target,devices,{"HDMI input"},{});assert(choice.state==Match::Ready&&choice.output==-1);
    Recovery recovery;assert(!recovery.pending()&&!recovery.due(100));recovery.begin(target,10);assert(!recovery.due(10.9)&&recovery.due(11));
    recovery.failed(11,"Device is busy");assert(recovery.detail=="Device is busy");assert(!recovery.due(12.9)&&recovery.due(13));recovery.failed(13);assert(!recovery.due(16.9)&&recovery.due(17));
    recovery.failed(17);assert(!recovery.due(24.9)&&recovery.due(25));recovery.failed(25);assert(!recovery.due(32.9)&&recovery.due(33));
    recovery.unavailable(Match::InputMissing,33);assert(recovery.detail.empty()&&recovery.state==Match::InputMissing&&!recovery.due(33.9)&&recovery.due(34));
    recovery.retryNow(33.5);assert(recovery.due(33.5));recovery.cancel();assert(!recovery.pending()&&!recovery.due(100));
    assert(helpfulError("No working 1920x1080 at 60/59.94 fps mode.").find("USB 3.0")!=std::string::npos);
    assert(helpfulError("Cannot open camera").find("permissions")!=std::string::npos);
    std::cout<<"PASS reconnect identity, alternate-device rejection, ambiguity, audio matching, default output, backoff, cancellation and helpful status\n";
}
