#pragma once
// Windows-only report formatting. No work is done from video or audio callbacks.
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

struct WindowsSessionDiagnostics {
    double durationSeconds=0;
    std::string videoMode, audioMode, schedulingMode;
    std::uint64_t arrived=0, arrivalsOver25ms=0, pendingDiscarded=0;
    std::uint64_t decoded=0, skippedBeforeUpload=0, invalid=0, displayed=0;
    std::uint64_t audioOverflowSamples=0, audioUnderflowSamples=0;
    double uploadAgeMeanMs=0, uploadAgeMaxMs=0;
    double presentCallMeanMs=0, presentCallMaxMs=0;
    unsigned uploadDestinations=0;
};

inline std::string formatWindowsSessionDiagnostics(const WindowsSessionDiagnostics &d){
    std::ostringstream s;
    s<<std::fixed<<std::setprecision(2);
    s<<"XR1-L Player | Windows capture session diagnostics\n"
     <<"Session duration: "<<d.durationSeconds<<" seconds\n"
     <<"Video mode: "<<d.videoMode<<"\n"
     <<"Audio mode: "<<d.audioMode<<"\n"
     <<"Playback scheduling: "<<d.schedulingMode<<"\n"
     <<"GPU upload destinations: "<<d.uploadDestinations<<"\n\n"
     <<"VIDEO\n"
     <<"Frames arriving at Windows capture callback: "<<d.arrived<<"\n"
     <<"Arrival gaps over 25 ms: "<<d.arrivalsOver25ms<<"\n"
     <<"Frames discarded before decode (newer sample replaced pending): "<<d.pendingDiscarded<<"\n"
     <<"Frames successfully decoded/published: "<<d.decoded<<"\n"
     <<"Decoded frames skipped before upload (newest-frame handoff): "<<d.skippedBeforeUpload<<"\n"
     <<"Invalid frames: "<<d.invalid<<"\n"
     <<"Frames submitted to renderer: "<<d.displayed<<"\n"
     <<"Frame ready-to-upload age, mean/max: "<<d.uploadAgeMeanMs<<" / "<<d.uploadAgeMaxMs<<" ms\n"
     <<"Render-present call time, mean/max: "<<d.presentCallMeanMs<<" / "<<d.presentCallMaxMs<<" ms\n\n"
     <<"AUDIO\n"
     <<"Overflow samples: "<<d.audioOverflowSamples<<"\n"
     <<"Underflow samples: "<<d.audioUnderflowSamples<<"\n\n"
     <<"NOTES\n"
     <<"Arrival gaps indicate timing variation, not necessarily dropped frames.\n"
     <<"Discarded frames are intentional latest-frame replacements to keep latency low.\n"
     <<"Audio counts are individual 16-bit channel samples (2 per stereo frame).\n"
     <<"Renderer submissions are not proof a monitor displayed each frame.\n"
     <<"This report does not measure true HDMI-to-screen latency or audio/video sync.\n";
    return s.str();
}

inline bool saveWindowsSessionDiagnostics(const std::filesystem::path &file,
                                          const WindowsSessionDiagnostics &report){
    if(file.empty())return false;
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(),ec);
    if(ec)return false;
    std::ofstream out(file,std::ios::binary|std::ios::trunc);
    if(!out)return false;
    out<<formatWindowsSessionDiagnostics(report);
    out.flush();
    return bool(out);
}
