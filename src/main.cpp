#include <iostream>
#include "video.hpp"
#include "audio.hpp"
#include "ui.hpp"
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <csignal>
#ifdef _WIN32
#include "windows_fps.hpp"
#include "windows_presenter.hpp"
#endif
struct Options {
#ifdef _WIN32
    bool fpsTest=false;
#endif
    bool demo=false,fullscreen=false,vsync=false,diagnose=false,autostart=false,reduceMotion=false;
    bool videoSet=false,audioSet=false,outputSet=false;
#ifdef _WIN32
    int seconds=0,latency=32,buffers=2;
#else
    int seconds=0,latency=20,buffers=2;
#endif
    std::string video,audio,output,format="auto",screenshot;
};
static std::filesystem::path settingsPath() {
    const char *config=std::getenv("XDG_CONFIG_HOME");
    if(config&&*config) return std::filesystem::path(config)/"xr1-play/settings";
#ifdef _WIN32
    if(const wchar_t *appdata=_wgetenv(L"APPDATA"))return std::filesystem::path(appdata)/L"XR1 Play/settings";
#endif
    const char *home=std::getenv("HOME");
    return home&&*home?std::filesystem::path(home)/".config/xr1-play/settings":std::filesystem::path{};
}
static void loadSettings(Options &o) {
    auto path=settingsPath();if(path.empty())return;
    std::ifstream in(path);std::string key,value;
    while(in>>key>>std::quoted(value)) {
        if(key=="video")o.video=value;
        if(key=="audio")o.audio=value;
        if(key=="output")o.output=value;
        if(key=="format")o.format=value;
    }
}
static void saveSettings(const std::string &video,const std::string &audio,const std::string &output,const std::string &format) {
    auto path=settingsPath();if(path.empty())return;
    std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);if(ec)return;
    std::ofstream out(path);out<<"video "<<std::quoted(video)<<"\naudio "<<std::quoted(audio)<<"\noutput "<<std::quoted(output)<<"\nformat "<<std::quoted(format)<<"\n";
}
static volatile std::sig_atomic_t interrupted=0;
static void handleSignal(int) {interrupted=1;}
static void help() {
    std::cout<<
#ifdef _WIN32
    "XR1 Play 1.5.3 - native 1080p60 HDMI viewer ("
#else
    "XR1 Play 1.5.0 - native 1080p60 HDMI viewer ("
#endif
#ifdef _WIN32
    "Windows x86_64"
#else
    "Linux x86_64"
#endif
    ")\n"
    "Usage: xr1-play [options]\n"
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
#ifdef _WIN32
    "  --fps-test             Start preview with a 30-second FPS test (F8)\n"
#endif
    "  --diagnose             Print devices and advertised 1080p frame rates\n"
    "  --seconds N            Exit after N seconds (testing)\n"
    "  --screenshot FILE.bmp  Save a screenshot before exit with --seconds\n"
    "Move the mouse for volume/fullscreen controls; double-click video for fullscreen.\n"
    "Keys: Enter start/stop, F/F11 fullscreen, Tab settings/stats, M mute, +/- volume,\n"
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
#ifdef _WIN32
        else if(a=="--fps-test") {o.fpsTest=true;o.autostart=true;}
#endif
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
    // X11 compositors can honor this for fullscreen windows. Wayland compositors
    // control their own presentation policy; this is not a bypass guarantee.
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR,"1");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)<0) throw std::runtime_error(SDL_GetError());
    if(TTF_Init()<0) throw std::runtime_error(TTF_GetError());
    SDL_Window *window=SDL_CreateWindow("XR1 Play",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1100,ui::height,SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
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
        shared.wakeEvent=SDL_RegisterEvents(1);
        if(shared.wakeEvent==static_cast<Uint32>(-1))throw std::runtime_error("Cannot register frame-ready event.");
        Frame uploadFrame;
        SDL_Event pendingEvent{};bool havePendingEvent=false;
        std::vector<VideoDevice> videos;std::vector<std::string> inputs,outputs;
        const std::vector<std::string> formats={"auto","yuyv","nv12","uyvy","i420","yv12","mjpeg"};
        int vi=0,ai=-1,oi=-1,fi=static_cast<int>(std::find(formats.begin(),formats.end(),o.format)-formats.begin());
        bool playing=false,menu=true,quit=false,demonstration=o.demo;
        bool reducedMotion=o.reduceMotion;
        bool validateVideo=o.videoSet,validateAudio=o.audioSet,validateOutput=o.outputSet;
        std::string error;
        double began=monotime(),startTime=began,lastStat=began,lastDemo=0,lastPaint=0;
        double controlsUntil=began+3;
        bool draggingVolume=false;
        int mouseX=-1,mouseY=-1;
        auto hover=[&](ui::Box box){int y=mouseY;if(menu&&box.y>=100)y-=ui::menuOffset;else if(!menu&&box.y>=600)y-=ui::toolbarOffset;return box.contains(mouseX,y);};
        int cursorState=SDL_ENABLE;
        uint64_t prevFrames=0,presented=0,prevPresented=0;
#ifdef _WIN32
        WindowsFpsTest fpsTest;bool fpsReportSaved=false;
        WindowsVideoUpload videoUploads;
        WindowsPlaybackScheduling playbackScheduling;
        WindowsFrameRate shownCadence,demoCadence;double arrivalFps=0;
        auto fpsCounts=[&](){auto native=capture.snapshot();return WindowsFpsCounters{demonstration?shared.captured.load():native.samples,shared.captured.load(),presented,shared.skipped.load(),native.pendingDropped,shared.corrupt.load(),native.late,audio.ring.dropped.load(),audio.ring.starved.load()};};
        auto saveFpsReport=[&](){
            if(!fpsTest.complete||fpsReportSaved)return;
            auto report=fpsTest.report()+"Mode: "+(demonstration?"test picture":shared.mode)+"\nRenderer: "+info.name+"\nAudio: "+audio.mode+"\n";
            report+="Video upload destinations: "+std::to_string(videoUploads.destinations())+"\nScheduling: "+playbackScheduling.mode()+"\n";
            SDL_DisplayMode display{};if(SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window),&display)==0)report+="Desktop refresh reported by SDL: "+std::to_string(display.refresh_rate)+" Hz\n";
            auto path=settingsPath().parent_path()/"windows-fps-test.txt";
            std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);
            std::ofstream out(path);out<<report;
            std::cout<<report<<"FPS report "<<(out?"saved: ":"could not be saved: ")<<path.string()<<std::endl;
            fpsReportSaved=true;
        };
#endif
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
        auto changeVolume=[&](float gain) {audio.volume=std::clamp(gain,0.0f,1.0f);audio.muted=false;controlsUntil=monotime()+3;};
        auto mute=[&]() {audio.muted=!audio.muted;controlsUntil=monotime()+3;std::cout<<"Muted: "<<(audio.muted?"yes":"no")<<std::endl;};
        auto refresh=[&]() {
            videos=videoDevices();inputs=audioDevices(true);outputs=audioDevices(false);vi=0;ai=-1;oi=-1;
            for(size_t i=0;i<videos.size();++i)if(isXr1(videos[i])){vi=static_cast<int>(i);break;}
            for(size_t i=0;i<videos.size();++i)if(!o.video.empty()&&(sameVideoPath(videos[i].path,o.video)||videos[i].name==o.video)){if(o.videoSet||isXr1(videos[i])||!isXr1(videos[static_cast<size_t>(vi)]))vi=static_cast<int>(i);break;}
            for(size_t i=0;i<inputs.size();++i) if((!o.audio.empty()&&inputs[i]==o.audio)||(o.audio.empty()&&xrname(inputs[i]))) {ai=static_cast<int>(i);break;}
            for(size_t i=0;i<outputs.size();++i) if(outputs[i]==o.output) oi=static_cast<int>(i);
        };
        refresh();
        auto stop=[&]() {
#ifdef _WIN32
            if(fpsTest.active){fpsTest.finish(monotime(),fpsCounts());saveFpsReport();}
            playbackScheduling.stop();
#endif
            capture.stop();audio.stop();playing=false;menu=true;draggingVolume=false;SDL_CaptureMouse(SDL_FALSE);cursor(true);};
        auto start=[&]() {
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
                    if(validateAudio&&(ai<0||inputs[static_cast<size_t>(ai)]!=o.audio))throw std::runtime_error("The requested audio input was not found. Choose the XR1 recording/input device.");
                    if(validateOutput&&(oi<0||outputs[static_cast<size_t>(oi)]!=o.output))throw std::runtime_error("The requested audio output was not found. Choose speakers or headphones.");
                    if(ai<0) throw std::runtime_error("Choose the XR1 audio input; it may be named USB Digital Audio. Enable the card's input/recording profile in Sound settings and allow desktop apps to use audio input.");
                    capture.start(videos[static_cast<size_t>(vi)].path,formats[static_cast<size_t>(fi)],static_cast<unsigned>(o.buffers));
                }
                audio.start(demonstration?nullptr:inputs[static_cast<size_t>(ai)].c_str(),oi<0?nullptr:outputs[static_cast<size_t>(oi)].c_str(),demonstration);
                playing=true;menu=false;startTime=monotime();lastStat=startTime;lastDemo=startTime-1.0/60;prevFrames=shared.captured;prevPresented=presented;controlsUntil=startTime+3;cursor(true);
#ifdef _WIN32
                shownCadence.reset();demoCadence.reset();arrivalFps=0;
#endif
                if(!demonstration)saveSettings(videos[static_cast<size_t>(vi)].path,inputs[static_cast<size_t>(ai)],oi<0?"":outputs[static_cast<size_t>(oi)],formats[static_cast<size_t>(fi)]);
                std::cout<<(demonstration?"DEMO - synthetic 1080p60":shared.mode)<<"\nAudio: "<<audio.mode<<"\n";
            } catch(const std::exception &e) {stop();error=e.what();std::cerr<<error<<"\n";if(o.autostart) result=1;}
        };
#ifdef _WIN32
        auto startFpsTest=[&](){if(!playing)start();if(playing){fpsTest.start(monotime(),fpsCounts(),demonstration);fpsReportSaved=false;menu=false;controlsUntil=monotime()+3;}};
        auto closeFpsTest=[&](){fpsTest.close(monotime(),fpsCounts());saveFpsReport();controlsUntil=monotime()+3;};
        auto toggleFpsTest=[&](){if(fpsTest.visible&&(fpsTest.active||fpsTest.complete))closeFpsTest();else startFpsTest();};
#endif
        if(o.autostart||o.demo) start();
#ifdef _WIN32
        if(o.fpsTest)startFpsTest();
#endif
        auto color=ui::white;auto dim=ui::muted;auto accent=ui::green;
        while(!quit&&!interrupted) {
            SDL_Event event;
            while(havePendingEvent||SDL_PollEvent(&event)) {
                if(havePendingEvent){event=pendingEvent;havePendingEvent=false;}
                if(event.type==SDL_QUIT) quit=true;
                if(event.type==SDL_AUDIODEVICEREMOVED&&playing&&audio.ownsDevice(event.adevice.which)) {stop();error="A selected audio device was removed. Refresh devices with R and reconnect.";}
#ifdef _WIN32
                if(event.type==SDL_KEYDOWN&&!event.key.repeat&&event.key.keysym.sym==SDLK_F8)toggleFpsTest();
                if(event.type==SDL_RENDER_DEVICE_RESET||event.type==SDL_RENDER_TARGETS_RESET){videoUploads.reset();texture=nullptr;}
#endif
                if(event.type==SDL_MOUSEMOTION) {
                    mouseX=event.motion.x;mouseY=event.motion.y;
                    controlsUntil=monotime()+3;
                    if(draggingVolume)changeVolume((event.motion.x-(menu?float(ui::volumeLeft):335.0f))/(menu?float(ui::volumeWidth):300.0f));
                }
                if(event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT&&draggingVolume) {
                    draggingVolume=false;SDL_CaptureMouse(SDL_FALSE);
                    std::cout<<"Volume: "<<static_cast<int>(std::lround(audio.volume*100))<<"%"<<std::endl;
                }
                if(event.type==SDL_KEYDOWN&&!event.key.repeat) {
                    auto key=event.key.keysym.sym;
                    if(key==SDLK_f||key==SDLK_F11) fullscreen();
                    if(key==SDLK_TAB&&playing) {menu=!menu;controlsUntil=monotime()+3;}
                    if(key==SDLK_m) mute();
                    if(key==SDLK_PLUS||key==SDLK_EQUALS||key==SDLK_KP_PLUS) changeVolume(audio.volume.load()+0.05f);
                    if(key==SDLK_MINUS||key==SDLK_KP_MINUS) changeVolume(audio.volume.load()-0.05f);
                    if(key==SDLK_r&&!playing) refresh();
                    if(key==SDLK_RETURN) {if(playing)stop();else start();}
                    if(key==SDLK_ESCAPE) {if(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP) fullscreen();else if(playing)stop();else quit=true;}
                }
                if(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT) {
                    int x=event.button.x,y=event.button.y;
                    if(menu&&y>=96)y-=ui::menuOffset;else if(!menu&&y>=580)y-=ui::toolbarOffset;
                    auto inside=[&](int left,int top,int width,int height){return x>=left&&x<left+width&&y>=top&&y<top+height;};
                    bool controls=menu||monotime()<controlsUntil;
                    controlsUntil=monotime()+3;
#ifdef _WIN32
                    if(!menu&&fpsTest.visible&&ui::Box{744,68,66,34}.contains(x,y)){closeFpsTest();continue;}
                    if(menu?ui::Box{470,38,152,42}.contains(x,y):(!fpsTest.visible&&controls&&ui::Box{22,62,152,34}.contains(x,y))){toggleFpsTest();continue;}
#endif
                    if(menu&&ui::fullscreen.contains(x,y)) {fullscreen();continue;}
                    if(menu&&ui::motion.contains(x,y)){reducedMotion=!reducedMotion;std::cout<<"Menu motion: "<<(reducedMotion?"off":"on")<<std::endl;continue;}
                    if(controls&&(menu?ui::mute.contains(x,y):inside(220,654,90,40))) {mute();continue;}
                    if(controls&&(menu?ui::volumeHit.contains(x,y):inside(325,650,320,44))) {
                        draggingVolume=true;SDL_CaptureMouse(SDL_TRUE);changeVolume((x-(menu?float(ui::volumeLeft):335.0f))/(menu?float(ui::volumeWidth):300.0f));continue;
                    }
                    if(playing&&!menu&&controls) {
                        if(inside(64,654,140,40)) {menu=true;continue;}
                        if(inside(710,654,92,40)) {stop();continue;}
                        if(inside(820,654,214,40)) {fullscreen();continue;}
                    }
                    if(playing&&!menu&&event.button.clicks==2) {fullscreen();continue;}
                    if(!menu)continue;
                    if(ui::primary.contains(x,y)) {if(playing){menu=false;controlsUntil=monotime()+3;}else start();continue;}
                    if(playing&&ui::stop.contains(x,y)){stop();continue;}
                    if(!playing) {
                        if(ui::refresh.contains(x,y)){refresh();continue;}
                        int dir=x>=580&&x<620?-1:1;
                        if(ui::devices[0].contains(x,y)&&!videos.empty()) {vi=(vi+dir+static_cast<int>(videos.size()))%static_cast<int>(videos.size());validateVideo=false;}
                        if(ui::devices[1].contains(x,y)&&!inputs.empty()) {ai=(ai+1+dir+static_cast<int>(inputs.size()+1))%static_cast<int>(inputs.size()+1)-1;validateAudio=false;}
                        if(ui::devices[2].contains(x,y)) {oi=(oi+1+dir+static_cast<int>(outputs.size()+1))%static_cast<int>(outputs.size()+1)-1;validateOutput=false;}
                        if(ui::format.contains(x,y))fi=(fi+1)%static_cast<int>(formats.size());
                        if(ui::demo.contains(x,y))demonstration=!demonstration;
                    }
                }
            }
            double now=monotime();bool newFrame=false;
            double uiInterval=menu?(reducedMotion?1.0/20:1.0/30):1.0/10;
            cursor(menu||now<controlsUntil||draggingVolume);
            if(playing&&demonstration&&now-lastDemo>=1.0/60) {Frame f;demoFrame(f,now-startTime);shared.publish(f);
#ifdef _WIN32
                demoCadence.record(monotime());
#endif
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
                double uploadBegan=monotime();texture=videoUploads.upload(renderer,f);
                fpsTest.recordUpload((monotime()-uploadBegan)*1000);
                if(!texture)streamError="Cannot upload video texture: "+std::string(SDL_GetError());
#else
                if(!texture||texturePixel!=f.pixel) {if(texture) SDL_DestroyTexture(texture); texture=SDL_CreateTexture(renderer,f.pixel,SDL_TEXTUREACCESS_STREAMING,1920,1080);texturePixel=f.pixel;if(texture)SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);}
                if(!texture||SDL_UpdateTexture(texture,nullptr,f.data.data(),f.pitch)<0) streamError="Cannot upload video texture: "+std::string(SDL_GetError());
#endif
                else {uploadAge=(monotime()-f.received)*1000;uploadTotal+=uploadAge;uploadMax=std::max(uploadMax,uploadAge);++uploadSamples;newFrame=true;}
            }
            if(playing&&!streamError.empty()) {stop();error=streamError;std::cerr<<error<<"\n";result=1;}
#ifdef _WIN32
            if(fpsTest.active){fpsTest.update(monotime(),fpsCounts());saveFpsReport();}
#endif
            if(now-lastStat>=1) {double elapsed=now-lastStat;measuredFps=static_cast<double>(shared.captured-prevFrames)/elapsed;displayFps=static_cast<double>(presented-prevPresented)/elapsed;prevFrames=shared.captured;prevPresented=presented;lastStat=now;}
#ifdef _WIN32
            arrivalFps=demonstration?demoCadence.fps(monotime()):capture.snapshot().fps;
            measuredFps=arrivalFps;displayFps=shownCadence.fps(monotime());
#endif
            bool timedExit=o.seconds&&now-began>=o.seconds;
            if(newFrame || now-lastPaint>uiInterval || timedExit) {
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
                if(menu) {
                    // Console-inspired dashboard. Solid surfaces keep device names
                    // readable; a small live preview replaces the old busy backdrop.
                    p.gradient({0,0,1100,ui::height},{18,21,26,255},{10,12,16,255});
                    double drift=reducedMotion?0:std::sin((now-began)*.3)*18;
                    p.glow({float(650+drift),-170,600,600},{155,240,11,35});
                    p.glow({float(-210-drift),400,520,520},{44,137,230,23});
                    p.round({40,32,48,48},13,accent);p.text("XR",46,41,{15,27,6,255},p.heading);
                    p.text("XR1 Play",102,32,color,p.heading);p.text("CAPTURE VIEWER",103,62,dim,p.tiny);
                    std::string status=playing?(demonstration?"TEST PICTURE":"LIVE PREVIEW"):videos.empty()?"CONNECT YOUR XR1":"READY TO PLAY";
                    p.round({688,42,184,34},17,{30,37,32,255});
                    auto liveDot=playing||!videos.empty()?accent:dim;
                    if(playing&&!reducedMotion)liveDot.a=static_cast<Uint8>(190+std::sin((now-began)*3)*50);
                    p.round({703,55,8,8},4,liveDot);p.text(status,721,51,playing||!videos.empty()?accent:dim,p.tiny);
#ifdef _WIN32
                    p.button({470,38,152,42},fpsTest.visible&&(fpsTest.active||fpsTest.complete)?"Close FPS test":"FPS test (F8)",hover({470,38,152,42}));
#endif
                    p.button(ui::fullscreen,(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)?"Exit fullscreen":"Fullscreen",hover(ui::fullscreen));
                    if(!error.empty()){
                        p.card({40,96,1020,42},{48,32,28,255},{110,69,43,255},10);
                        p.fit(error,54,108,992,{244,185,139,255},p.small);
                    }
                    p.offsetY=ui::menuOffset;
                    p.card({40,222,650,312},ui::surface,ui::border,18);
                    p.icon(ui::Icon::Video,60,242,23,accent);p.text("Connections",95,240,color,p.heading);
                    if(!playing){p.button(ui::refresh,"Refresh",hover(ui::refresh));}
                    else {p.round({573,240,96,26},13,{35,50,26,255});p.text("CONNECTED",584,246,accent,p.tiny);}
                    auto deviceRow=[&](int index,const std::string &label,const std::string &value,ui::Icon icon){
                        auto box=ui::devices[static_cast<size_t>(index)];bool hot=!playing&&hover(box);
                        float focus=p.animate(100+index,hot);
                        p.card(box,Painter::mix(ui::raised,{41,49,57,255},focus),Painter::mix(ui::border,accent,focus),10);
                        p.icon(icon,box.x+15,box.y+24,24,hot?accent:dim);
                        p.text(label,box.x+54,box.y+10,dim,p.tiny);p.fit(value,box.x+54,box.y+29,455,color);
                        if(!playing){p.icon(ui::Icon::Chevron,585,box.y+24,18,dim,true);p.icon(ui::Icon::Chevron,631,box.y+24,18,accent);}
                    };
                    deviceRow(0,"VIDEO SOURCE",videos.empty()?"Connect your XR1 Lite":videos[static_cast<size_t>(vi)].name,ui::Icon::Video);
                    deviceRow(1,"HDMI AUDIO INPUT",ai<0?"Choose your capture card audio":inputs[static_cast<size_t>(ai)],ui::Icon::Speaker);
                    deviceRow(2,"PLAYBACK DEVICE",oi<0?"System default output":outputs[static_cast<size_t>(oi)],ui::Icon::Speaker);
                    p.card({714,222,346,312},playing?ui::surface:accent,playing?ui::border:accent,18);
                    if(playing&&texture){
                        SDL_Rect preview{730,238+ui::menuOffset,314,177};SDL_RenderCopy(renderer,texture,nullptr,&preview);if(newFrame)++presented;
                    }else{
                        SDL_Rect clip{726,234+ui::menuOffset,322,288};SDL_RenderSetClipRect(renderer,&clip);
                        for(int i=0;i<5;++i)p.line(650.0f+i*100,230,930.0f+i*100,535,{10,34,0,13},38);
                        p.text("PLAY",740,260,{10,34,0,17},p.display);SDL_RenderSetClipRect(renderer,nullptr);
                        int bob=reducedMotion?0:static_cast<int>(std::lround(std::sin((now-began)*1.1)*3));
                        double tilt=reducedMotion?-10:-10+std::sin((now-began)*.7)*2;
                        p.icon(ui::Icon::Controller,786,250+bob+7,212,{12,32,0,24},false,tilt);
                        p.icon(ui::Icon::Controller,780,244+bob,212,{12,30,2,255},false,tilt);
                    }
                    if(playing){
                        p.text("Preview",734,431,color,p.heading);
                        p.fit(shared.mode,734,465,302,dim,p.small);
                    }
                    p.round({734,493,117,25},7,playing?SDL_Color{40,61,28,255}:SDL_Color{16,34,4,255});p.text("1080p / 60 fps",746,498,accent,p.tiny);
                    p.round({861,493,176,25},7,playing?SDL_Color{32,43,27,255}:SDL_Color{16,34,4,255});p.text("STEREO HDMI AUDIO",873,498,playing?dim:accent,p.tiny);
                    p.text("Video format",40,565,dim,p.small);
                    p.button(ui::format,fi==0?"Automatic":formats[static_cast<size_t>(fi)],!playing&&hover(ui::format));
                    p.card(ui::demo,hover(ui::demo)&&!playing?SDL_Color{43,53,35,255}:ui::raised,demonstration?accent:ui::border,10);
                    p.round({328,565,18,18},5,demonstration?accent:SDL_Color{62,73,53,255});
                    if(demonstration)p.icon(ui::Icon::Check,330,567,14,{15,29,7,255});
                    p.text("Test picture",359,563,color);
                    p.text("Volume",550,565,dim,p.small);
                    p.button(ui::motion,reducedMotion?"Motion: Off":"Motion: On",hover(ui::motion),false,p.small);
                    p.button(ui::primary,playing?"Back to game":"Start playing",hover(ui::primary),true);
                    p.icon(ui::Icon::Play,275,639,22,{15,29,7,255});
                    if(playing)p.button(ui::stop,"Stop preview",hover(ui::stop));
                    else {p.text("Enter  Start playing",736,628,dim,p.small);p.text("F11  Fullscreen     M  Mute",736,651,dim,p.small);}
                    if(playing){std::ostringstream stats;stats<<std::fixed<<std::setprecision(1)<<"Capture "<<measuredFps<<" fps   /   Display "<<displayFps<<" fps   /   Audio queued "<<audio.ring.queued()/96<<" ms   /   Skipped "<<shared.skipped<<"   /   Invalid "<<shared.corrupt;p.text(stats.str(),40,697,dim,p.tiny);}
                    else p.text("DIRECT CAPTURE    /    1080p60    /    STEREO SOUND",40,697,dim,p.tiny);
#ifdef _WIN32
                    p.text("XR1 PLAY 1.5.3",954,697,dim,p.tiny);
#else
                    p.text("XR1 PLAY 1.5",969,697,dim,p.tiny);
#endif
                } else {
                    if(demonstration){p.round({22,20,380,34},10,{16,23,15,220});p.text("TEST PICTURE  /  1080p60 + stereo tone",36,29,color,p.small);}
                    if(audio.muted){p.round({952,20,122,34},10,{16,23,15,220});p.icon(ui::Icon::Speaker,964,28,18,dim,true);p.text("Muted",992,29,dim,p.small);}
                }
#ifdef _WIN32
                if(playing&&!menu){
                    p.offsetY=0;
                    std::ostringstream live;live<<std::fixed<<std::setprecision(1)<<"Capture "<<arrivalFps<<" fps  /  Shown "<<displayFps<<" fps  /  Audio "<<audio.ring.queued()/96<<" ms";
                    if(fpsTest.visible){
                        p.round({22,62,800,54},10,{16,23,15,225});p.text(live.str(),36,69,color,p.small);
                        p.text(fpsTest.active||fpsTest.complete?fpsTest.summary():"F8: run a 30-second FPS test",36,91,dim,p.tiny);
                        p.button({744,68,66,34},"Close",hover({744,68,66,34}),false,p.small);
                    }else if(now<controlsUntil)p.button({22,62,152,34},"FPS test (F8)",hover({22,62,152,34}),false,p.small);
                }
#endif
                if(menu||now<controlsUntil||draggingVolume) {
                    p.offsetY=menu?ui::menuOffset:ui::toolbarOffset;
                    if(!menu){
                        p.round({50,644,1000,60},16,{17,23,15,242});
                        p.button({64,654,140,40},"Settings",hover({64,654,140,40}));
                        p.button({710,654,92,40},"Stop",hover({710,654,92,40}));
                        p.button({820,654,214,40},(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP)?"Exit fullscreen":"Fullscreen",hover({820,654,214,40}));
                    }
                    int left=menu?ui::volumeLeft:335,top=menu?ui::volumeTop:670,width=menu?ui::volumeWidth:300;
                    if(menu){p.card(ui::mute,hover(ui::mute)?SDL_Color{48,58,44,255}:ui::raised,hover(ui::mute)?accent:ui::border,10);p.icon(ui::Icon::Speaker,630,563,22,audio.muted?dim:accent,audio.muted);}
                    else p.button({220,654,90,40},audio.muted?"Unmute":"Mute",hover({220,654,90,40}));
                    p.round({left,top,width,6},3,{67,79,57,255});
                    int fill=static_cast<int>(std::lround(audio.volume*width));
                    if(fill>0)p.round({left,top,fill,6},3,accent);
                    p.round({left+fill-7,top-4,14,14},7,color);
                    p.text(std::to_string(static_cast<int>(std::lround(audio.volume*100)))+"%",menu?895:650,menu?562:664,dim);
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
#ifdef _WIN32
                fpsTest.recordPresent(presentMs);
                if(newFrame&&playing&&texture)shownCadence.record(monotime());
#endif
                presentTotal+=presentMs;presentMax=std::max(presentMax,presentMs);++presentSamples;
                lastPaint=now;
            }
            if(timedExit) quit=true;
            // Sleep until a frame or UI event arrives, instead of polling at
            // 1 ms intervals. Keep timer deadlines for demo/UI/automatic exit.
            double deadline=lastPaint+uiInterval;
            if(playing&&demonstration)deadline=std::min(deadline,lastDemo+1.0/60);
            if(o.seconds)deadline=std::min(deadline,began+o.seconds);
            int timeout=std::clamp(static_cast<int>(std::ceil((deadline-monotime())*1000)),1,100);
            if(!quit&&!interrupted)havePendingEvent=SDL_WaitEventTimeout(&pendingEvent,timeout)==1;
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
    catch(const std::exception &e) {std::cerr<<"XR1 Play: "<<e.what()<<"\n";
#ifdef _WIN32
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"XR1 Play",e.what(),nullptr);
#endif
        return 1;}
}
