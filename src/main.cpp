#include <iostream>
#include "video.hpp"
#include "audio.hpp"
#include "ui.hpp"
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <csignal>
#include <initializer_list>
#include <utility>
#include "menu_ui.hpp"
#include "frame_cadence.hpp"
#include "connection_recovery.hpp"
#ifdef _WIN32
#include "windows_presenter.hpp"
#endif
struct Options {
    bool colorPreference=false,muted=false;
    float volume=0.7f;
    ui::ConsoleColor colorTheme=ui::ConsoleColor::Xbox;
    bool demo=false,fullscreen=false,vsync=false,diagnose=false,autostart=false,reduceMotion=false;
    bool videoSet=false,audioSet=false,outputSet=false;
#ifdef _WIN32
    int seconds=0,latency=32,buffers=2;
#else
    int seconds=0,latency=20,buffers=2;
#endif
    std::string video,audio,output,format="auto",screenshot;
};
static std::filesystem::path settingsPath(bool legacy=false) {
    const char *config=std::getenv("XDG_CONFIG_HOME");
    if(config&&*config) return std::filesystem::path(config)/(legacy?"xr1-play/settings":"xr1-l-player/settings");
#ifdef _WIN32
    if(const wchar_t *appdata=_wgetenv(L"APPDATA"))return std::filesystem::path(appdata)/(legacy?L"XR1 Play/settings":L"XR1-L Player/settings");
#endif
    const char *home=std::getenv("HOME");
    return home&&*home?std::filesystem::path(home)/(legacy?".config/xr1-play/settings":".config/xr1-l-player/settings"):std::filesystem::path{};
}
static std::filesystem::path existingSettingsPath(){
    auto path=settingsPath();std::error_code ec;
    return !path.empty()&&std::filesystem::exists(path,ec)?path:settingsPath(true);
}
static void loadSettings(Options &o) {
    auto path=existingSettingsPath();if(path.empty())return;
    std::ifstream in(path);std::string key,value;
    while(in>>key>>std::quoted(value)) {
        if(key=="video")o.video=value;
        if(key=="audio")o.audio=value;
        if(key=="output")o.output=value;
        if(key=="format")o.format=value;
        if(key=="theme")o.colorTheme=ui::parseColor(value);
        if(key=="volume")try{size_t used=0;float gain=std::stof(value,&used);if(used==value.size()&&std::isfinite(gain)&&gain>=0&&gain<=1)o.volume=gain;}catch(const std::exception &){}
        if(key=="motion")o.reduceMotion=value=="off";
        if(key=="muted"){if(value=="true"||value=="1")o.muted=true;else if(value=="false"||value=="0")o.muted=false;}
    }
}
// Merge preference updates so volume, theme and device choices never erase
// one another. Replace a complete file only after a successful write/close.
static bool updateSettings(std::initializer_list<std::pair<std::string,std::string>> updates){
    auto path=settingsPath();if(path.empty())return false;
    std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);if(ec)return false;
    auto source=existingSettingsPath();bool exists=std::filesystem::exists(source,ec);if(ec)return false;
    std::ifstream in(source);if(exists&&!in)return false;
    std::ostringstream contents;std::string line;
    while(std::getline(in,line)){
        std::istringstream field(line);std::string key;field>>key;bool replaced=false;
        for(const auto &item:updates)if(key==item.first)replaced=true;
        if(!replaced)contents<<line<<"\n";
    }
    if(in.bad())return false;
    in.close();for(const auto &item:updates)contents<<item.first<<" "<<std::quoted(item.second)<<"\n";
#ifdef _WIN32
    auto id=GetCurrentProcessId();
#else
    auto id=getpid();
#endif
    auto temporary=path;temporary+=".tmp."+std::to_string(id);
    std::ofstream out(temporary);out<<contents.str();out.flush();out.close();
    if(!out){std::filesystem::remove(temporary,ec);return false;}
#ifdef _WIN32
    if(MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING))return true;
#else
    std::filesystem::rename(temporary,path,ec);if(!ec)return true;
#endif
    std::filesystem::remove(temporary,ec);return false;
}
static bool saveSettings(const std::string &video,const std::string &audio,const std::string &output,const std::string &format,ui::ConsoleColor colorTheme,float volume,bool muted){
    return updateSettings({{"video",video},{"audio",audio},{"output",output},{"format",format},{"theme",ui::colorKey(colorTheme)},{"volume",std::to_string(volume)},{"muted",muted?"true":"false"}});
}
static bool saveColorPreference(ui::ConsoleColor choice){return updateSettings({{"theme",ui::colorKey(choice)}});}
static bool saveMotionPreference(bool reduced){return updateSettings({{"motion",reduced?"off":"on"}});}
static bool saveVolumePreference(float volume,bool muted){return updateSettings({{"volume",std::to_string(volume)},{"muted",muted?"true":"false"}});}
static volatile std::sig_atomic_t interrupted=0;
static void handleSignal(int) {interrupted=1;}
static void help() {
    std::cout<<
#ifdef _WIN32
    "XR1-L Player 1.5.17 - native 1080p60 HDMI viewer ("
#else
    "XR1-L Player 1.5.17 Linux test build - native 1080p60 HDMI viewer ("
#endif
#ifdef _WIN32
    "Windows x86_64"
#else
    "Linux x86_64"
#endif
    ")\n"
    "Usage: xr1-l-player [options]\n"
    "  --demo                 Animated 1080p60 test picture + 440 Hz stereo tone\n"
    "  --video ID_OR_NAME     Select a video device from --diagnose\n"
    "  --audio NAME           Exact capture-device name from --diagnose\n"
    "  --output NAME          Exact playback-device name (default: system default)\n"
    "  --format auto|yuyv|nv12|uyvy|i420|yv12|mjpeg\n"
#ifdef _WIN32
    "  --audio-buffer MS      Audio queue cap, 5..80 ms (default 32; reserve ~13 ms)\n"
#else
    "  --audio-buffer MS      App audio queue cap, 5..80 ms (default 20)\n"
#endif
    "  --capture-buffers N    Requested V4L2 buffers, 2..8 (default 2)\n"
    "  --start                Start the selected devices immediately\n"
    "  --fullscreen           Open fullscreen\n"
    "  --vsync                Enable vsync (default off; may reduce tearing)\n"
    "  --reduce-motion        Disable decorative menu animation\n"
    "  --color-preference     Open the in-app color preference panel\n"
    "  --diagnose             Print devices and advertised 1080p frame rates\n"
    "  --seconds N            Exit after N seconds (testing)\n"
    "  --screenshot FILE.bmp  Save a screenshot before exit with --seconds\n"
    "Move the mouse for volume/fullscreen controls; double-click video for fullscreen.\n"
    "Keys: Enter start/stop, F/F11 fullscreen, Tab settings / return to video, M mute, +/- volume,\n"
    "      R refresh devices, Esc leave fullscreen / stop / quit.\n";
}
static Options parse(int argc,char **argv) {
    Options o;loadSettings(o);
    for(int i=1;i<argc;++i) {
        std::string a=argv[i];
        auto value=[&]() {if(i+1>=argc) throw std::runtime_error("Missing value for "+a);return std::string(argv[++i]);};
        if(a=="--help"||a=="-h") {help();std::exit(0);}
        else if(a=="--demo") o.demo=true;
        else if(a=="--fullscreen") o.fullscreen=true;
        else if(a=="--color-preference")o.colorPreference=true;
        else if(a=="--vsync") o.vsync=true;
        else if(a=="--reduce-motion")o.reduceMotion=true;
        else if(a=="--diagnose") o.diagnose=true;
        else if(a=="--start") o.autostart=true;
        else if(a=="--video") {o.video=value();o.videoSet=true;}
        else if(a=="--audio") {o.audio=value();o.audioSet=true;}
        else if(a=="--output") {o.output=value();o.outputSet=true;}
        else if(a=="--format") o.format=value();
        else if(a=="--seconds") o.seconds=std::stoi(value());
        else if(a=="--audio-buffer") o.latency=std::stoi(value());
        else if(a=="--capture-buffers") o.buffers=std::stoi(value());
        else if(a=="--screenshot") o.screenshot=value();
        else throw std::runtime_error("Unknown option: "+a);
    }
    if(o.latency<5||o.latency>80||o.seconds<0) throw std::runtime_error("Audio buffer must be 5..80 ms; seconds must be nonnegative.");
    if(o.buffers<2||o.buffers>8)throw std::runtime_error("Capture buffers must be 2..8.");
    if(o.format!="auto"&&o.format!="yuyv"&&o.format!="nv12"&&o.format!="uyvy"&&o.format!="i420"&&o.format!="yv12"&&o.format!="mjpeg") throw std::runtime_error("Invalid pixel format.");
    return o;
}
static std::vector<std::string> audioDevices(bool capture) {
    std::vector<std::string> result;
    for(int i=0;i<SDL_GetNumAudioDevices(capture);++i) {const char *s=SDL_GetAudioDeviceName(i,capture); if(s) result.emplace_back(s);}
    return result;
}
static void diagnoseAudio() {
    std::cout<<"Audio driver: "<<(SDL_GetCurrentAudioDriver()?SDL_GetCurrentAudioDriver():"unavailable")<<"\n";
    for(bool input:{true,false}) {std::cout<<(input?"Audio inputs:\n":"Audio outputs:\n");for(auto &s:audioDevices(input))std::cout<<"  "<<s<<"\n";}
}
static void demoFrame(Frame &f,double t) {
    f.pixel=SDL_PIXELFORMAT_RGB24;f.pitch=5760;f.data.resize(5760*1080);
    const uint8_t colors[8][3]={{220,230,235},{220,200,55},{60,200,195},{65,185,95},{190,70,190},{205,65,65},{65,100,210},{18,24,35}};
    for(int y=0;y<1080;++y) for(int x=0;x<1920;++x) {
        auto *p=&f.data[static_cast<size_t>(y*1920+x)*3];int bar=x/240;
        bool marker=std::abs(x-static_cast<int>(std::fmod(t*480,1920)))<12;
        for(int k=0;k<3;++k) p[k]=marker?255:static_cast<uint8_t>(colors[bar][k]*(y>820?0.3:1.0));
    }
    f.received=monotime();
}
static int app(const Options &o) {
    SDL_SetMainReady();
    // PipeWire/PulseAudio manage device sharing and resampling on Bazzite.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");
    SDL_SetHint(SDL_HINT_APP_NAME,"XR1-L Player");
    SDL_setenv("SDL_VIDEO_X11_WMCLASS","xr1-l-player",0);
    // X11 compositors can honor this for fullscreen windows. Wayland compositors
    // control their own presentation policy; this is not a bypass guarantee.
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR,"1");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)<0) throw std::runtime_error(SDL_GetError());
    if(TTF_Init()<0) throw std::runtime_error(TTF_GetError());
    SDL_Window *window=SDL_CreateWindow("XR1-L Player",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1100,ui::height,SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
    if(!window) throw std::runtime_error(SDL_GetError());
    SDL_SetWindowMinimumSize(window,900,560);
#ifdef _WIN32
    // SDL2's default D3D9 path can fall back to CPU YUV conversion. Prefer
    // D3D11's NV12/I420 shaders, retaining other accelerated backends as fallback.
    SDL_Renderer *renderer=nullptr;
    for(int i=0;i<SDL_GetNumRenderDrivers();++i){SDL_RendererInfo candidate{};if(SDL_GetRenderDriverInfo(i,&candidate)==0&&std::string(candidate.name)=="direct3d11"){renderer=SDL_CreateRenderer(window,i,SDL_RENDERER_ACCELERATED|(o.vsync?SDL_RENDERER_PRESENTVSYNC:0));break;}}
    if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|(o.vsync?SDL_RENDERER_PRESENTVSYNC:0));
#else
    SDL_Renderer *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|(o.vsync?SDL_RENDERER_PRESENTVSYNC:0));
#endif
    if(!renderer) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer) throw std::runtime_error(SDL_GetError());
    int syncResult=SDL_RenderSetVSync(renderer,o.vsync?1:0);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer,1100,ui::height);
    if(o.fullscreen&&SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP)<0)throw std::runtime_error("Cannot enter fullscreen: "+std::string(SDL_GetError()));
    SDL_RendererInfo info{};SDL_GetRendererInfo(renderer,&info);std::cout<<"Renderer: "<<info.name<<" / video driver: "<<SDL_GetCurrentVideoDriver()<<" / vsync requested: "<<o.vsync<<" / renderer accepted: "<<(syncResult==0?"yes":"no")<<"\n";
    int result=0;
    {
        Painter p(renderer);SharedVideo shared;Capture capture(shared);Audio audio(o.latency);
        audio.volume=o.volume;audio.muted=o.muted;
        // Native capture wakes this thread directly. SDL still handles window
        // and input events; no extra video-event relay is needed.
        Frame uploadFrame;
        SDL_Event pendingEvent{};bool havePendingEvent=false;
        std::vector<VideoDevice> videos;std::vector<std::string> inputs,outputs;
        const std::vector<std::string> formats={"auto","yuyv","nv12","uyvy","i420","yv12","mjpeg"};
        int vi=0,ai=-1,oi=-1,fi=static_cast<int>(std::find(formats.begin(),formats.end(),o.format)-formats.begin());
        bool playing=false,menu=true,quit=false,demonstration=o.demo;
        connection::Recovery recovery;std::optional<connection::Target> lastTarget;
        bool resumeMenu=false;ui::Screen resumeScreen=ui::Screen::Main;
        double lastVideoAt=0,lastDeviceRefresh=monotime();
        bool reducedMotion=o.reduceMotion;
        bool validateVideo=o.videoSet,validateAudio=o.audioSet,validateOutput=o.outputSet;
        std::string error;
        double began=monotime(),startTime=began,lastStat=began,lastDemo=0,lastPaint=0;
        double controlsUntil=began+3;
        bool draggingVolume=false;
        int mouseX=-1,mouseY=-1;
        auto hover=[&](ui::Box box){int y=mouseY;if(!menu&&box.y>=600)y-=ui::toolbarOffset;return box.contains(mouseX,y);};
        ui::Screen screen=o.colorPreference?ui::Screen::Preferences:ui::Screen::Main;
        int menuFocus=0;bool keyboardNavigation=false;
        int cursorState=SDL_ENABLE;
        ui::MenuFrameClock menuClock;int menuRefresh=60;
        auto refreshMenuRate=[&](){SDL_DisplayMode mode{};int display=SDL_GetWindowDisplayIndex(window);menuRefresh=display>=0&&SDL_GetCurrentDisplayMode(display,&mode)==0?mode.refresh_rate:60;};
        refreshMenuRate();
        uint64_t prevFrames=0,presented=0,prevPresented=0;
        auto colorTheme=o.colorTheme;std::string colorSaveError;
        auto chooseColor=[&](int index){colorTheme=static_cast<ui::ConsoleColor>(index);p.theme.choice=colorTheme;p.setWindowIcon(window);p.clearLabels();colorSaveError=saveColorPreference(colorTheme)?"":"Could not save this preference. Your color is applied for this session.";std::cout<<"Color: "<<ui::themeName(colorTheme)<<std::endl;};
        p.theme.choice=colorTheme;p.setWindowIcon(window);
#ifdef _WIN32
        WindowsVideoUpload videoUploads;
        WindowsPlaybackScheduling playbackScheduling;
#endif
        FrameRate shownCadence,demoCadence;double arrivalFps=0;
        double measuredFps=0,displayFps=0,uploadAge=0;
        double uploadTotal=0,uploadMax=0,presentTotal=0,presentMax=0;
        uint64_t uploadSamples=0,presentSamples=0;
        SDL_Texture *texture=nullptr;
#ifndef _WIN32
        Uint32 texturePixel=0;
#endif
        auto cursor=[&](bool visible) {int state=visible?SDL_ENABLE:SDL_DISABLE;if(state!=cursorState){SDL_ShowCursor(state);cursorState=state;}};
        auto fullscreen=[&]() {
            bool active=(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)!=0;
            if(SDL_SetWindowFullscreen(window,active?0:SDL_WINDOW_FULLSCREEN_DESKTOP)<0)error="Cannot toggle fullscreen: "+std::string(SDL_GetError());
            else std::cout<<"Fullscreen: "<<(active?"off":"on")<<std::endl;
            controlsUntil=monotime()+3;
        };
        bool volumeDirty=false;double volumeSaveAfter=0;
        const std::string volumeSaveError="Volume could not be saved. Check the settings folder permissions and free space.";
        auto persistVolume=[&](){if(volumeDirty){
            if(saveVolumePreference(audio.volume.load(),audio.muted.load())){
                volumeDirty=false;if(error==volumeSaveError)error.clear();
            }else{
                error=volumeSaveError;volumeSaveAfter=monotime()+2;
            }
        }};
        auto volumeChanged=[&](){volumeDirty=true;volumeSaveAfter=monotime()+.35;};
        auto changeVolume=[&](float gain) {volumeChanged();audio.volume=std::clamp(gain,0.0f,1.0f);audio.muted=false;controlsUntil=monotime()+3;};
        auto mute=[&]() {volumeChanged();audio.muted=!audio.muted;controlsUntil=monotime()+3;std::cout<<"Muted: "<<(audio.muted?"yes":"no")<<std::endl;};
        auto refresh=[&](bool preserve=false) {
            std::string selectedVideo=preserve&&vi>=0&&size_t(vi)<videos.size()?videos[size_t(vi)].path:"";
            std::string selectedInput=preserve&&ai>=0&&size_t(ai)<inputs.size()?inputs[size_t(ai)]:"";
            std::string selectedOutput=preserve&&oi>=0&&size_t(oi)<outputs.size()?outputs[size_t(oi)]:"";
            bool defaultOutput=preserve&&oi<0;
            videos=videoDevices();inputs=audioDevices(true);outputs=audioDevices(false);vi=0;ai=-1;oi=-1;
            for(size_t i=0;i<videos.size();++i)if(isXr1(videos[i])){vi=static_cast<int>(i);break;}
            for(size_t i=0;i<videos.size();++i)if(!o.video.empty()&&(sameVideoPath(videos[i].path,o.video)||videos[i].name==o.video)){if(o.videoSet||isXr1(videos[i])||!isXr1(videos[static_cast<size_t>(vi)]))vi=static_cast<int>(i);break;}
            for(size_t i=0;i<inputs.size();++i) if((!o.audio.empty()&&inputs[i]==o.audio)||(o.audio.empty()&&xrname(inputs[i]))) {ai=static_cast<int>(i);break;}
            for(size_t i=0;i<outputs.size();++i) if(outputs[i]==o.output) oi=static_cast<int>(i);
            if(preserve){
                for(size_t i=0;i<videos.size();++i)if(videos[i].path==selectedVideo)vi=int(i);
                for(size_t i=0;i<inputs.size();++i)if(inputs[i]==selectedInput)ai=int(i);
                if(defaultOutput)oi=-1;else for(size_t i=0;i<outputs.size();++i)if(outputs[i]==selectedOutput)oi=int(i);
            }
        };
        refresh();
        auto stop=[&](bool cancelRecovery=true) {
            if(cancelRecovery){recovery.cancel();error.clear();lastDeviceRefresh=0;}
            lastVideoAt=0;
            persistVolume();
#ifdef _WIN32
            playbackScheduling.stop();
#endif
            capture.stop();audio.stop();
            {std::lock_guard<std::mutex> lock(shared.mutex);shared.error.clear();shared.dirty=false;}
#ifdef _WIN32
            videoUploads.reset();
#else
            if(texture)SDL_DestroyTexture(texture);
            texturePixel=0;
#endif
            texture=nullptr;playing=false;menu=true;screen=ui::Screen::Main;menuFocus=0;draggingVolume=false;SDL_CaptureMouse(SDL_FALSE);cursor(true);};
        auto start=[&](bool automatic=false) {
            if(!automatic)recovery.cancel();
            error.clear();
            try {
#ifdef _WIN32
                playbackScheduling.start();
#endif
                if(demonstration) {
                    std::lock_guard<std::mutex> lock(shared.mutex);
                    shared.error.clear();shared.dirty=false;shared.captured=0;shared.skipped=0;shared.corrupt=0;
                }
                if(!demonstration) {
                    if(videos.empty()) throw std::runtime_error("No capture card found. Connect the XR1 Lite to USB 3, close other capture apps, then press R.");
                    if(validateVideo&&!sameVideoPath(videos[static_cast<size_t>(vi)].path,o.video)&&videos[static_cast<size_t>(vi)].name!=o.video)throw std::runtime_error("The requested video device was not found. Refresh or choose a connected capture device.");
                    if(validateAudio&&(ai<0||inputs[static_cast<size_t>(ai)]!=o.audio))throw std::runtime_error("The requested audio input was not found. Choose the XR1 Lite recording/input device.");
                    if(validateOutput&&(oi<0||outputs[static_cast<size_t>(oi)]!=o.output))throw std::runtime_error("The requested audio output was not found. Choose speakers or headphones.");
                    if(ai<0) throw std::runtime_error("Choose the XR1 Lite audio input; it may be named USB Digital Audio. Enable the card's input/recording profile in Sound settings and allow desktop apps to use audio input.");
                    lastTarget=connection::Target{videos[size_t(vi)],inputs[size_t(ai)],oi<0?"":outputs[size_t(oi)],formats[size_t(fi)]};
                    capture.start(videos[static_cast<size_t>(vi)].path,formats[static_cast<size_t>(fi)],static_cast<unsigned>(o.buffers));
                }
                audio.start(demonstration?nullptr:inputs[static_cast<size_t>(ai)].c_str(),oi<0?nullptr:outputs[static_cast<size_t>(oi)].c_str(),demonstration);
                playing=true;menu=false;startTime=monotime();lastVideoAt=0;lastStat=startTime;lastDemo=startTime-1.0/60;prevFrames=shared.captured;presented=prevPresented=0;controlsUntil=startTime+3;cursor(true);
                measuredFps=displayFps=uploadAge=0;uploadTotal=uploadMax=presentTotal=presentMax=0;uploadSamples=presentSamples=0;
                shownCadence.reset();demoCadence.reset();arrivalFps=0;
                if(!demonstration&&!saveSettings(videos[static_cast<size_t>(vi)].path,inputs[static_cast<size_t>(ai)],oi<0?"":outputs[static_cast<size_t>(oi)],formats[static_cast<size_t>(fi)]
                    ,colorTheme,audio.volume.load(),audio.muted.load()
                ))error="Settings could not be saved. Check the settings folder permissions and free space.";
                std::cout<<(demonstration?"DEMO - synthetic 1080p60":shared.mode)<<"\nAudio: "<<audio.mode<<"\n";
            } catch(const std::exception &e) {stop(!automatic);error=connection::helpfulError(e.what());std::cerr<<e.what()<<"\n";if(o.autostart&&!automatic) result=1;}
        };
        auto recover=[&](const std::string &reason,connection::Match cause=connection::Match::Interrupted){
            if(demonstration||!lastTarget){stop();error=connection::helpfulError(reason);return;}
            auto target=*lastTarget;resumeMenu=menu;resumeScreen=screen;
            stop();recovery.begin(std::move(target),monotime());recovery.state=cause;error.clear();
            if(resumeMenu)screen=resumeScreen;
            std::cerr<<"Connection interrupted: "<<reason<<"; automatic reconnect enabled\n";
        };
        auto openScreen=[&](ui::Screen next){if(recovery.pending()){resumeMenu=true;resumeScreen=next;}screen=next;menu=true;menuFocus=0;keyboardNavigation=false;controlsUntil=monotime()+3;
            std::cout<<"Menu: "<<(screen==ui::Screen::Main?"main":screen==ui::Screen::Settings?"settings":"preferences")<<std::endl;};
        auto toggleMotion=[&](){reducedMotion=!reducedMotion;colorSaveError=saveMotionPreference(reducedMotion)?"":"Could not save motion preference. It is applied for this session.";std::cout<<"Menu motion: "<<(reducedMotion?"off":"on")<<std::endl;};
        auto settingAction=[&](int index,int dir){
            if(index<=4&&recovery.pending()){recovery.cancel();lastDeviceRefresh=0;}
            if(index<=4&&playing){error="Stop playback to change capture devices or format.";return;}
            if(index==0&&!videos.empty()){vi=(vi+dir+static_cast<int>(videos.size()))%static_cast<int>(videos.size());validateVideo=false;}
            else if(index==1)fi=(fi+dir+static_cast<int>(formats.size()))%static_cast<int>(formats.size());
            else if(index==2)demonstration=!demonstration;
            else if(index==3&&!inputs.empty()){ai=(ai+1+dir+static_cast<int>(inputs.size()+1))%static_cast<int>(inputs.size()+1)-1;validateAudio=false;}
            else if(index==4){oi=(oi+1+dir+static_cast<int>(outputs.size()+1))%static_cast<int>(outputs.size()+1)-1;validateOutput=false;}
            else if(index==5)mute();
            else if(index==6)changeVolume(audio.volume.load()+dir*.05f);
            else if(index==7&&!playing){refresh(true);recovery.retryNow(monotime());}
            else if(index==8)fullscreen();
            else if(index==9){if(playing)menu=false;else if(recovery.pending())recovery.retryNow(monotime());else start();}
            else if(index==10&&(playing||recovery.pending()))stop();
            else if(index==11)openScreen(ui::Screen::Main);
        };
        auto menuAction=[&](int index,int dir){
            if(screen==ui::Screen::Main){
                if(index==0){if(recovery.pending())stop();else if(playing){menu=false;controlsUntil=monotime()+3;}else start();}
                else openScreen(index==1?ui::Screen::Settings:ui::Screen::Preferences);
            }else if(screen==ui::Screen::Settings)settingAction(index,dir);
            else if(index<3)chooseColor(index);
            else if(index==3)toggleMotion();
            else if(index==4)fullscreen();
            else openScreen(ui::Screen::Main);
        };
        if(o.autostart||o.demo) start();
        if(o.colorPreference)openScreen(ui::Screen::Preferences);
        auto color=ui::white;auto dim=ui::muted;auto accent=ui::green;
        while(!quit&&!interrupted) {
            SDL_Event event;
            while(havePendingEvent||SDL_PollEvent(&event)) {
                if(havePendingEvent){event=pendingEvent;havePendingEvent=false;}
                if(event.type==SDL_QUIT) quit=true;
                if(event.type==SDL_DISPLAYEVENT||(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_DISPLAY_CHANGED))refreshMenuRate();
                if(event.type==SDL_AUDIODEVICEREMOVED&&playing&&audio.ownsDevice(event.adevice.which))recover("A selected audio device was disconnected.",event.adevice.iscapture?connection::Match::InputMissing:connection::Match::OutputMissing);
                if(event.type==SDL_AUDIODEVICEADDED||event.type==SDL_AUDIODEVICEREMOVED)lastDeviceRefresh=0;
                if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_LOST&&draggingVolume){draggingVolume=false;SDL_CaptureMouse(SDL_FALSE);persistVolume();}
                if(event.type==SDL_RENDER_DEVICE_RESET||event.type==SDL_RENDER_TARGETS_RESET){
#ifdef _WIN32
                    videoUploads.reset();
#else
                    if(texture)SDL_DestroyTexture(texture);
#endif
                    texture=nullptr;p.resetTextures();
                }
                if(event.type==SDL_MOUSEMOTION) {
                    mouseX=event.motion.x;mouseY=event.motion.y;keyboardNavigation=false;
                    controlsUntil=monotime()+3;
                    if(draggingVolume)changeVolume((event.motion.x-(menu?float(ui::volumeLeft):float(ui::playbackVolumeLeft)))/(menu?float(ui::volumeWidth):float(ui::playbackVolumeWidth)));
                }
                if(event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT&&draggingVolume) {
                    draggingVolume=false;SDL_CaptureMouse(SDL_FALSE);persistVolume();
                    std::cout<<"Volume: "<<static_cast<int>(std::lround(audio.volume*100))<<"%"<<std::endl;
                }
                if(event.type==SDL_KEYDOWN&&!event.key.repeat) {
                    auto key=event.key.keysym.sym;
                    if(key==SDLK_f||key==SDLK_F11)fullscreen();
                    if(key==SDLK_m)mute();
                    if(key==SDLK_PLUS||key==SDLK_EQUALS||key==SDLK_KP_PLUS)changeVolume(audio.volume.load()+.05f);
                    if(key==SDLK_MINUS||key==SDLK_KP_MINUS)changeVolume(audio.volume.load()-.05f);
                    if(key==SDLK_r&&!playing){refresh(true);recovery.retryNow(monotime());}
                    if(key==SDLK_ESCAPE){
                        if(menu&&screen!=ui::Screen::Main)openScreen(ui::Screen::Main);
                        else if(recovery.pending())stop();
                        else if(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)fullscreen();
                        else if(playing)stop();else quit=true;
                    }else if(key==SDLK_TAB&&playing){if(menu)menu=false;else openScreen(ui::Screen::Settings);controlsUntil=monotime()+3;}
                    else if(menu){
                        int count=screen==ui::Screen::Main?3:screen==ui::Screen::Settings?12:6;
                        if(key==SDLK_UP||key==SDLK_DOWN||key==SDLK_TAB){
                            int dir=key==SDLK_UP||(key==SDLK_TAB&&(event.key.keysym.mod&KMOD_SHIFT))?-1:1;
                            menuFocus=(menuFocus+count+dir)%count;
                            if(screen==ui::Screen::Settings&&!playing&&!recovery.pending()&&menuFocus==10)menuFocus=(menuFocus+count+dir)%count;
                            keyboardNavigation=true;
                        }else if(key==SDLK_LEFT||key==SDLK_RIGHT){
                            keyboardNavigation=true;int dir=key==SDLK_LEFT?-1:1;
                            if(screen==ui::Screen::Settings&&menuFocus<=6)settingAction(menuFocus,dir);
                            else if(screen==ui::Screen::Preferences&&menuFocus<3)menuFocus=(menuFocus+3+dir)%3;
                        }else if(key==SDLK_RETURN||key==SDLK_SPACE)menuAction(menuFocus,1);
                    }else if(key==SDLK_RETURN)stop();
                }
                if(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT){
                    int x=event.button.x,y=event.button.y;keyboardNavigation=false;
                    if(!menu&&y>=580)y-=ui::toolbarOffset;
                    bool controls=menu||monotime()<controlsUntil;controlsUntil=monotime()+3;
                    bool volumeVisible=menu?screen==ui::Screen::Settings:controls;
                    if(volumeVisible&&(menu?ui::mute.contains(x,y):ui::playbackMute.contains(x,y))){mute();continue;}
                    if(volumeVisible&&(menu?ui::volumeHit.contains(x,y):ui::playbackVolumeHit.contains(x,y))){
                        draggingVolume=true;SDL_CaptureMouse(SDL_TRUE);changeVolume((x-(menu?float(ui::volumeLeft):float(ui::playbackVolumeLeft)))/(menu?float(ui::volumeWidth):float(ui::playbackVolumeWidth)));continue;
                    }
                    if(playing&&!menu&&controls){
                        if(ui::playbackMain.contains(x,y)){openScreen(ui::Screen::Main);continue;}
                        if(ui::playbackSettings.contains(x,y)){openScreen(ui::Screen::Settings);continue;}
                        if(ui::playbackStop.contains(x,y)){stop();continue;}
                        if(ui::playbackFullscreen.contains(x,y)){fullscreen();continue;}
                    }
                    if(playing&&!menu&&event.button.clicks==2){fullscreen();continue;}
                    if(!menu)continue;
                    if(screen==ui::Screen::Main){for(int i=0;i<3;++i)if(ui::navigation[size_t(i)].contains(x,y)){menuFocus=i;menuAction(i,1);break;}}
                    else if(screen==ui::Screen::Settings){for(int i=0;i<12;++i){auto box=ui::settingControls[size_t(i)];if(box.contains(x,y)){menuFocus=i;menuAction(i,x>=box.x+box.w-90&&x<box.x+box.w-44?-1:1);break;}}}
                    else {for(int i=0;i<6;++i)if(ui::preferenceControls[size_t(i)].contains(x,y)){menuFocus=i;menuAction(i,1);break;}}
                }
            }
            double now=monotime();bool newFrame=false;
            if(volumeDirty&&!draggingVolume&&now>=volumeSaveAfter)persistVolume();
            if(!quit&&!interrupted&&recovery.due(now)){
                try{
                    auto freshVideos=videoDevices();auto freshInputs=audioDevices(true),freshOutputs=audioDevices(false);
                    auto choice=connection::resolve(recovery.target(),freshVideos,freshInputs,freshOutputs);
                    if(choice.state==connection::Match::Ready){
                        auto target=recovery.target();videos=std::move(freshVideos);inputs=std::move(freshInputs);outputs=std::move(freshOutputs);
                        vi=choice.video;ai=choice.input;oi=choice.output;fi=int(std::find(formats.begin(),formats.end(),target.format)-formats.begin());
                        validateVideo=validateAudio=validateOutput=false;start(true);
                        if(playing){recovery.cancel();menu=resumeMenu;screen=resumeScreen;result=0;std::cout<<"Connection restored\n";}
                        else {recovery.failed(monotime(),error);error.clear();if(resumeMenu)screen=resumeScreen;}
                    }else recovery.unavailable(choice.state,now);
                }catch(const std::exception &e){recovery.failed(monotime(),connection::helpfulError(e.what()));std::cerr<<"Reconnect attempt: "<<e.what()<<"\n";}
            }else if(!playing&&!recovery.pending()&&now-lastDeviceRefresh>=2){
                // Device discovery stays out of the active playback path.
                try{refresh(true);}catch(const std::exception &e){error=connection::helpfulError(e.what());}
                lastDeviceRefresh=now;
            }
            double uiInterval=1.0/10;
            if(menu)menuClock.rate(ui::menuFrameRate(menuRefresh,reducedMotion,playing));
            accent=p.theme.accent();
            cursor(menu||now<controlsUntil||draggingVolume);
            if(playing&&demonstration&&now-lastDemo>=1.0/60) {Frame f;demoFrame(f,now-startTime);shared.publish(f);
                demoCadence.record(monotime());
                lastDemo+=1.0/60;if(now-lastDemo>0.1)lastDemo=now;}
            std::string streamError;
            {
                std::lock_guard<std::mutex> lock(shared.mutex);
                streamError=shared.error;
            }
            // Take ownership quickly. A GPU/driver stall must not stop capture
            // from publishing a newer frame and draining the card's buffers.
            if(playing&&shared.take(uploadFrame)) {
                auto &f=uploadFrame;
#ifdef _WIN32
                texture=videoUploads.upload(renderer,f);
                if(!texture)streamError="Cannot upload video texture: "+std::string(SDL_GetError());
#else
                if(!texture||texturePixel!=f.pixel) {if(texture) SDL_DestroyTexture(texture); texture=SDL_CreateTexture(renderer,f.pixel,SDL_TEXTUREACCESS_STREAMING,1920,1080);texturePixel=f.pixel;if(texture)SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);}
                if(!texture||SDL_UpdateTexture(texture,nullptr,f.data.data(),f.pitch)<0) streamError="Cannot upload video texture: "+std::string(SDL_GetError());
#endif
                else {
                    uploadAge=(monotime()-f.received)*1000;uploadTotal+=uploadAge;uploadMax=std::max(uploadMax,uploadAge);++uploadSamples;newFrame=true;lastVideoAt=now;}
            }
            if(playing&&!streamError.empty()) {
                if(streamError.find("Cannot upload video texture")!=std::string::npos){stop();error=streamError;result=1;}
                else recover(streamError,lower(streamError).find("frames")!=std::string::npos?connection::Match::VideoWaiting:lower(streamError).find("disconnected")!=std::string::npos?connection::Match::CardMissing:connection::Match::Interrupted);
            }
            if(now-lastStat>=1) {double elapsed=now-lastStat;measuredFps=static_cast<double>(shared.captured-prevFrames)/elapsed;displayFps=static_cast<double>(presented-prevPresented)/elapsed;prevFrames=shared.captured;prevPresented=presented;lastStat=now;}
#ifdef _WIN32
            arrivalFps=demonstration?demoCadence.fps(monotime()):capture.snapshot().fps;
#else
            arrivalFps=demonstration?demoCadence.fps(monotime()):shared.captureFps();
#endif
            measuredFps=arrivalFps;displayFps=shownCadence.fps(monotime());
            bool timedExit=o.seconds&&now-began>=o.seconds;
            if(newFrame || (menu?menuClock.due(now):now-lastPaint>uiInterval) || timedExit) {
                SDL_RenderSetLogicalSize(renderer,1100,ui::height);
                p.prepare();p.offsetY=0;
                p.motion=!reducedMotion;p.frame(now);
                SDL_SetRenderDrawColor(renderer,9,15,23,255);SDL_RenderClear(renderer);
                if(playing&&texture&&!menu) {
                    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);
                    SDL_RenderSetLogicalSize(renderer,0,0);
                    int vw=w,vh=static_cast<int>(w*9.0/16);if(vh>h) {vh=h;vw=static_cast<int>(h*16.0/9);}
                    SDL_Rect dst{(w-vw)/2,(h-vh)/2,vw,vh};SDL_RenderCopy(renderer,texture,nullptr,&dst);
                    SDL_RenderSetLogicalSize(renderer,1100,ui::height);
                    if(newFrame) ++presented;
                }
                if(menu){
                    ui::menuBackground(p);
                    auto focus=[&](ui::Box box,int index){return hover(box)||(keyboardNavigation&&menuFocus==index);};
                    ui::menuHeader(p,screen,hover(ui::back)||(keyboardNavigation&&menuFocus==(screen==ui::Screen::Settings?11:5)),colorTheme);
                    if(screen==ui::Screen::Main){
                        for(int i=0;i<3;++i)ui::navigationButton(p,i,i==0?(recovery.pending()?"CANCEL":"PLAY"):i==1?"SETTINGS":"PREFERENCES",focus(ui::navigation[size_t(i)],i));
                        if(ui::logoPanel(p,now-began,reducedMotion,playing?texture:nullptr)&&newFrame)++presented;
                        p.text("Up / Down  Choose     Enter  Select",40,605,dim,p.small);
                        p.text(recovery.pending()?"Esc  Cancel reconnect":playing?"Tab  Return to video":"Esc  Quit",548,605,dim,p.small);
                    }else if(screen==ui::Screen::Settings){
                        p.card({40,155,485,325},{13,20,24,245},{43,56,61,255},18);
                        p.card({575,155,485,325},{13,20,24,245},{43,56,61,255},18);
                        p.text("VIDEO SETTINGS",60,177,accent,p.small);p.text("AUDIO SETTINGS",595,177,accent,p.small);
                        auto deviceRow=[&](int index,const std::string &label,const std::string &value,int focusIndex,ui::Icon icon){
                            auto box=ui::devices[size_t(index)];bool hot=focus(box,focusIndex);float f=p.animate(100+index,hot);
                            p.card(box,Painter::mix(ui::raised,{35,46,53,255},f),hot?accent:ui::border,10);
                            p.icon(icon,box.x+14,box.y+23,22,playing?dim:accent);
                            p.text(label,box.x+48,box.y+10,dim,p.tiny);p.fit(value,box.x+48,box.y+30,298,color,p.small);
                            if(!playing){p.icon(ui::Icon::Chevron,box.x+box.w-76,box.y+23,18,dim,true);p.icon(ui::Icon::Chevron,box.x+box.w-32,box.y+23,18,accent);}
                        };
                        deviceRow(0,"CAPTURE DEVICE",videos.empty()?"Connect your XR1 Lite":videos[size_t(vi)].name,0,ui::Icon::Video);
                        deviceRow(1,"HDMI AUDIO INPUT",ai<0?"Choose capture card audio":inputs[size_t(ai)],3,ui::Icon::Speaker);
                        deviceRow(2,"PLAYBACK OUTPUT",oi<0?"System default output":outputs[size_t(oi)],4,ui::Icon::Speaker);
                        p.card(ui::format,ui::raised,focus(ui::format,1)?accent:ui::border,10);
                        p.text("VIDEO FORMAT",76,314,dim,p.tiny);p.text(fi==0?"Automatic":formats[size_t(fi)],76,333,color,p.small);
                        if(!playing){p.icon(ui::Icon::Chevron,429,324,18,dim,true);p.icon(ui::Icon::Chevron,473,324,18,accent);}
                        p.button(ui::demo,demonstration?"Test picture: On":"Test picture: Off",focus(ui::demo,2),false,p.small);
                        p.text(playing?"Stop playback to change devices.":"Changes apply when you press Play.",60,450,dim,p.tiny);
                        p.button(ui::refresh,"Refresh devices",focus(ui::refresh,7),false,p.small);
                        p.button(ui::fullscreen,(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)?"Exit fullscreen":"Fullscreen",focus(ui::fullscreen,8),false,p.small);
                        p.button(ui::primary,playing?"Resume":recovery.pending()?"Retry now":"Play",focus(ui::primary,9),true);
                        if(playing||recovery.pending())p.button(ui::stop,recovery.pending()?"Cancel reconnect":"Stop playback",focus(ui::stop,10),false,p.small);
                        p.text(playing?"Tab  Return to video     Esc  Main menu":"Arrows  Browse     Enter  Select     Esc  Main menu",40,605,dim,p.small);
                    }else{
                        p.text("COLOR PREFERENCE",40,143,dim,p.small);
                        for(int i=0;i<3;++i)ui::themeCard(p,i,static_cast<int>(colorTheme)==i,focus(ui::themes[size_t(i)],i));
                        bool hot=focus(ui::motion,3);p.card(ui::motion,{17,24,29,255},hot?accent:ui::border,16);
                        p.text("Motion",64,394,color,p.heading);p.text("Animate menu artwork",64,430,dim,p.small);
                        p.round({437,403,72,32},16,reducedMotion?ui::border:accent);
                        p.round({reducedMotion?442:481,408,22,22},11,reducedMotion?dim:p.theme.ink());
                        p.text(reducedMotion?"OFF":"ON",371,411,reducedMotion?dim:accent,p.small);
                        p.card(ui::preferenceFullscreen,{17,24,29,255},focus(ui::preferenceFullscreen,4)?accent:ui::border,16);
                        p.text("Fullscreen",599,394,color,p.heading);
                        p.text((SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)?"On  /  F11 to switch":"Off  /  F11 to switch",599,430,dim,p.small);
                        p.icon(ui::Icon::Expand,1004,405,27,accent);
                        p.fit(colorSaveError.empty()?"Preferences saved automatically.":colorSaveError,40,508,1020,colorSaveError.empty()?dim:SDL_Color{244,185,139,255},p.small);
                        p.text("Arrows  Browse     Enter  Select     Esc  Main menu",40,605,dim,p.small);
                    }
                    std::string status=recovery.pending()?"Reconnecting":playing?(demonstration?"Test picture":lastVideoAt&&now-lastVideoAt<1.5?"Live video":"Waiting for video"):videos.empty()?"Connect XR1 Lite":ai<0?"Select HDMI audio":"Card connected";
                    auto statusColor=recovery.pending()?SDL_Color{244,185,139,255}:playing&&lastVideoAt&&now-lastVideoAt<1.5?accent:dim;
                    p.card({408,34,376,36},{17,24,29,255},ui::border,12);p.round({422,48,8,8},4,statusColor);p.text(status,444,43,statusColor,p.small);
                    if(recovery.pending()&&error.empty()){p.card({40,102,1020,34},{30,28,23,255},{96,76,45,255},8);p.fit(recovery.detail.empty()?std::string(connection::waiting(recovery.state)):recovery.detail+" Retrying automatically.",52,111,992,{244,185,139,255},p.small);}
                    p.text("F11  Fullscreen",933,605,dim,p.small);
                    if(!error.empty()){p.card({40,102,1020,34},{43,28,23,255},{128,79,49,255},8);p.fit(error,52,111,992,{244,185,139,255},p.small);}
                } else {
                    if(demonstration){p.round({22,20,380,34},10,{16,23,15,220});p.text("TEST PICTURE  /  1080p60 + stereo tone",36,29,color,p.small);}
                    if(audio.muted){p.round({952,20,122,34},10,{16,23,15,220});p.icon(ui::Icon::Speaker,964,28,18,dim,true);p.text("Muted",992,29,dim,p.small);}
                }
                if(playing&&!menu&&!demonstration&&now-(lastVideoAt?lastVideoAt:startTime)>1.5){p.card({22,62,790,48},{24,25,23,245},{96,76,45,255},10);p.text("Waiting for video. Check console power and the HDMI connection.",36,77,{244,185,139,255},p.small);}
                if((menu&&screen==ui::Screen::Settings)||(!menu&&(now<controlsUntil||draggingVolume))) {
                    p.offsetY=menu?0:ui::toolbarOffset;
                    if(!menu){
                        p.round(ui::playbackToolbar,16,{17,23,15,242});
                        p.button(ui::playbackMain,"Main menu",hover(ui::playbackMain));
                        p.button(ui::playbackSettings,"Settings",hover(ui::playbackSettings));
                        p.button(ui::playbackStop,"Stop",hover(ui::playbackStop));
                        p.button(ui::playbackFullscreen,(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)?"Exit fullscreen":"Fullscreen",hover(ui::playbackFullscreen));
                    }
                    int left=menu?ui::volumeLeft:ui::playbackVolumeLeft,top=menu?ui::volumeTop:ui::playbackVolumeTop,width=menu?ui::volumeWidth:ui::playbackVolumeWidth;
                    if(menu){p.card(ui::mute,(hover(ui::mute)||(keyboardNavigation&&menuFocus==5))?SDL_Color{48,58,44,255}:ui::raised,(hover(ui::mute)||(keyboardNavigation&&menuFocus==5))?accent:ui::border,10);p.icon(ui::Icon::Speaker,606,409,22,audio.muted?dim:accent,audio.muted);p.text("VOLUME",650,382,dim,p.tiny);}
                    else p.button(ui::playbackMute,audio.muted?"Unmute":"Mute",hover(ui::playbackMute));
                    if(menu&&keyboardNavigation&&menuFocus==6)p.card(ui::volumeHit,{20,28,31,255},accent,8);
                    p.round({left,top,width,6},3,{67,79,57,255});
                    int fill=static_cast<int>(std::lround(audio.volume*width));
                    if(fill>0)p.round({left,top,fill,6},3,accent);
                    p.round({left+fill-7,top-4,14,14},7,color);
                    p.text(std::to_string(static_cast<int>(std::lround(audio.volume*100)))+"%",menu?930:636,menu?405:664,dim);
                }
                if(timedExit&&!o.screenshot.empty()) {
                    // Read the entire drawable, rather than the logical UI viewport.
                    SDL_RenderSetLogicalSize(renderer,0,0);
                    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);auto *surface=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_ARGB8888);
                    if(surface) {if(SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch)==0) SDL_SaveBMP(surface,o.screenshot.c_str());SDL_FreeSurface(surface);}
                    SDL_RenderSetLogicalSize(renderer,1100,ui::height);
                }
                double beforePresent=monotime();SDL_RenderPresent(renderer);
                double presentMs=(monotime()-beforePresent)*1000;
                if(newFrame&&playing&&texture)shownCadence.record(monotime());
                presentTotal+=presentMs;presentMax=std::max(presentMax,presentMs);++presentSamples;
                lastPaint=now;if(menu)menuClock.painted(now);
            }
            if(timedExit) quit=true;
            // Sleep until a frame or UI event arrives, instead of polling at
            // 1 ms intervals. Keep timer deadlines for demo/UI/automatic exit.
            double deadline=menu?menuClock.deadline(monotime()):lastPaint+uiInterval;
            if(playing&&demonstration)deadline=std::min(deadline,lastDemo+1.0/60);
            if(o.seconds)deadline=std::min(deadline,began+o.seconds);
            int timeout=std::clamp(static_cast<int>(std::ceil((deadline-monotime())*1000)),1,100);
            if(!quit&&!interrupted){
                if(playing&&!demonstration){
                    // A predicate protects frames published just before sleep.
                    // Poll UI events at least every 8 ms, even if HDMI is idle.
                    shared.waitForFrame(std::min(timeout,8));
                }else havePendingEvent=SDL_WaitEventTimeout(&pendingEvent,timeout)==1;
            }
        }
        std::cout<<std::fixed<<std::setprecision(2)<<"Frames received: "<<shared.captured<<" / displayed: "<<presented<<" / stale skipped: "<<shared.skipped<<" / invalid: "<<shared.corrupt<<"\nLast measured receive fps: "<<measuredFps<<" / display fps: "<<displayFps<<" / last CPU upload wait: "<<uploadAge<<" ms (not end-to-end latency)\nAudio overflow samples: "<<audio.ring.dropped<<" / underflow samples: "<<audio.ring.starved<<"\n";
        std::cout<<"Ready-to-upload mean/max: "<<(uploadSamples?uploadTotal/uploadSamples:0)<<" / "<<uploadMax<<" ms; present-call mean/max: "<<(presentSamples?presentTotal/presentSamples:0)<<" / "<<presentMax<<" ms. These exclude card/monitor latency.\n";
        stop();
#ifndef _WIN32
        if(texture) SDL_DestroyTexture(texture);
#endif
    }
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();return result;
}
int main(int argc,char **argv) {
    std::signal(SIGINT,handleSignal);std::signal(SIGTERM,handleSignal);
    try {auto options=parse(argc,argv);[[maybe_unused]] CapturePlatform platform;if(options.diagnose) {help();diagnoseVideo();SDL_SetMainReady();if(SDL_Init(SDL_INIT_AUDIO)<0) std::cerr<<"Audio initialization: "<<SDL_GetError()<<"\n";diagnoseAudio();SDL_Quit();return 0;}return app(options);}
    catch(const std::exception &e) {std::cerr<<"XR1-L Player: "<<e.what()<<"\n";
#ifdef _WIN32
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"XR1-L Player",e.what(),nullptr);
#endif
        return 1;}
}
