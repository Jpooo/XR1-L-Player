#include "../src/windows_diagnostics.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
int main(){
    WindowsSessionDiagnostics s;
    s.durationSeconds=60.25;
    s.videoMode="1080p60 NV12";
    s.audioMode="48 kHz stereo";
    s.schedulingMode="MMCSS Games / high";
    s.uploadDestinations=2;
    s.arrived=3620;
    s.arrivalsOver25ms=4;
    s.pendingDiscarded=2;
    s.decoded=3618;
    s.skippedBeforeUpload=3;
    s.invalid=0;
    s.displayed=3615;
    s.audioOverflowSamples=0;
    s.audioUnderflowSamples=14;
    s.uploadAgeMeanMs=1.25;
    s.uploadAgeMaxMs=13.2;
    s.presentCallMeanMs=0.35;
    s.presentCallMaxMs=6.5;
    std::string report=formatWindowsSessionDiagnostics(s);
    assert(report.find("Session duration: 60.25 seconds")!=std::string::npos);
    assert(report.find("Arrival gaps over 25 ms: 4")!=std::string::npos);
    assert(report.find("Frames discarded before decode (newer sample replaced pending): 2")!=std::string::npos);
    assert(report.find("Frames submitted to renderer: 3615")!=std::string::npos);
    assert(report.find("Underflow samples: 14")!=std::string::npos);
    assert(report.find("does not measure true HDMI-to-screen latency or audio/video sync")!=std::string::npos);
    const auto file=std::filesystem::temp_directory_path()/"xr1-windows-diagnostics-test"/"last-session-diagnostics.txt";
    assert(saveWindowsSessionDiagnostics(file,s));
    std::ifstream in(file,std::ios::binary);
    assert(in);
    std::string disk((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    assert(disk==report);
    in.close();
    std::error_code ec;
    std::filesystem::remove(file,ec);
    std::filesystem::remove(file.parent_path(),ec);
    std::cout<<"PASS Windows diagnostics formatting and saved report\n";
}
