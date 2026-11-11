#pragma once
#include "video_common.hpp"
#include <optional>

// Recovery is an application policy. Capture queues, mode negotiation and audio
// callbacks continue to use the existing native implementations.
namespace connection {
struct Target {VideoDevice video;std::string input,output,format;};
enum class Match {Ready,CardMissing,InputMissing,OutputMissing,Ambiguous,VideoWaiting,Interrupted};
struct Selection {Match state=Match::CardMissing;int video=-1,input=-1,output=-1;};
inline bool compatible(const VideoDevice &a,const VideoDevice &b){
    if(!a.usbId.empty()&&!b.usbId.empty())return lower(a.usbId)==lower(b.usbId);
    return a.name==b.name;
}
inline bool persistentPath(const std::string &path){auto p=lower(path);return p.find("/by-id/")!=std::string::npos||p.find("vid_")!=std::string::npos||p.find("@device:")!=std::string::npos;}
inline Selection resolve(const Target &target,const std::vector<VideoDevice> &videos,const std::vector<std::string> &inputs,const std::vector<std::string> &outputs){
    Selection result;
    auto same=[](const std::string &a,const std::string &b){
#ifdef _WIN32
        return lower(a)==lower(b);
#else
        return a==b;
#endif
    };
    for(size_t i=0;i<videos.size();++i)if(same(videos[i].path,target.video.path)&&compatible(videos[i],target.video)){result.video=int(i);break;}
    if(result.video<0&&!persistentPath(target.video.path)){
        for(size_t i=0;i<videos.size();++i)if(videos[i].name==target.video.name&&compatible(videos[i],target.video)){
            if(result.video>=0){result.state=Match::Ambiguous;return result;}result.video=int(i);
        }
    }
    if(result.video<0)return result;
    auto audio=[&](const std::vector<std::string> &list,const std::string &name,int &index){
        for(size_t i=0;i<list.size();++i)if(list[i]==name){if(index>=0)return false;index=int(i);}
        return true;
    };
    if(!audio(inputs,target.input,result.input)){result.state=Match::Ambiguous;return result;}
    if(result.input<0){result.state=Match::InputMissing;return result;}
    if(!target.output.empty()){
        if(!audio(outputs,target.output,result.output)){result.state=Match::Ambiguous;return result;}
        if(result.output<0){result.state=Match::OutputMissing;return result;}
    }
    result.state=Match::Ready;return result;
}
class Recovery {
    std::optional<Target> requested;
    double next=0;unsigned failures=0;
public:
    Match state=Match::CardMissing;std::string detail;
    bool pending()const{return requested.has_value();}
    const Target &target()const{return requested.value();}
    void begin(Target target,double now){requested=std::move(target);state=Match::CardMissing;failures=0;detail.clear();next=now+1;}
    void cancel(){requested.reset();failures=0;detail.clear();}
    bool due(double now)const{return pending()&&now>=next;}
    void retryNow(double now){if(pending())next=now;}
    void unavailable(Match value,double now){state=value;detail.clear();next=now+1;}
    void failed(double now,std::string cause={}){state=Match::Ready;detail=std::move(cause);next=now+std::min(8u,1u<<std::min(++failures,3u));}
};
inline const char *waiting(Match state){
    switch(state){
    case Match::Interrupted:return "Capture stopped. Retrying the selected devices automatically.";
    case Match::VideoWaiting:return "Video stopped arriving. Check console power and HDMI; reconnecting automatically.";
    case Match::CardMissing:return "Waiting for the selected capture card. Check USB; reconnecting automatically.";
    case Match::InputMissing:return "Waiting for the selected HDMI audio input to reconnect.";
    case Match::OutputMissing:return "Waiting for your selected speakers or headphones to reconnect.";
    case Match::Ambiguous:return "More than one matching device. Cancel reconnect and choose your devices in Settings.";
    default:return "Could not reopen the selected devices. Close other capture apps or check permissions; retrying.";
    }
}
inline std::string helpfulError(const std::string &message){
    auto value=lower(message);
    if(value.find("no capture card")!=std::string::npos||value.find("requested video device")!=std::string::npos)return "Capture card not found. Check its USB connection, then refresh devices.";
    if(value.find("audio input")!=std::string::npos||value.find("recording/input")!=std::string::npos)return "Choose the XR1 Lite HDMI audio input in Settings. Check microphone/input permission if it is missing.";
    if(value.find("audio output")!=std::string::npos||value.find("speakers")!=std::string::npos)return "Audio output unavailable. Reconnect your speakers or headphones, or choose another output in Settings.";
    if(value.find("60/59.94")!=std::string::npos||value.find("1080p60")!=std::string::npos||value.find("1080p progressive")!=std::string::npos)return "1080p60 was not accepted. Use a USB 3.0 port/data cable and set the console to 1080p60 SDR.";
    if(value.find("enumerate")!=std::string::npos)return "Cannot list capture devices. Check camera/input permissions and the USB connection.";
    if(value.find("cannot open")!=std::string::npos||value.find("camera")!=std::string::npos)return "Cannot open the capture device. Close other capture apps and check camera/input permissions.";
    return message;
}
}
