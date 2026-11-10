// Run the real player UI with timed input, without capture hardware.
#include <SDL.h>
static int injectedPoll(SDL_Event *);
#define SDL_PollEvent injectedPoll
#define main player_application_main
#include "../src/main.cpp"
#undef main
#undef SDL_PollEvent
#include <cassert>
struct Input {Uint32 at,type;SDL_Keycode key=0;};
static std::vector<Input> script;
static Uint32 pollBegan=0;
static size_t nextInput=0;
static bool firstPoll=true;
static int injectedPoll(SDL_Event *event){
    if(firstPoll){pollBegan=SDL_GetTicks();firstPoll=false;}
    if(SDL_PollEvent(event))return 1;
    if(nextInput==script.size()||SDL_GetTicks()-pollBegan<script[nextInput].at)return 0;
    const auto &input=script[nextInput++];*event={};event->type=input.type;
    if(input.type==SDL_KEYDOWN)event->key.keysym.sym=input.key;
    if(input.type==SDL_USEREVENT){Options saved;loadSettings(saved);assert(std::abs(saved.volume-.65f)<.0001f&&!saved.muted);}
    return 1;
}
int main(){
    [[maybe_unused]] CapturePlatform platform;
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
#ifndef _WIN32
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);
    auto id=getpid();
#else
    auto id=GetCurrentProcessId();
#endif
    auto folder=std::filesystem::temp_directory_path()/("xr1-player-ui-"+std::to_string(id)+"-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
#ifdef _WIN32
    _putenv_s("XDG_CONFIG_HOME",folder.string().c_str());
#else
    SDL_setenv("XDG_CONFIG_HOME",folder.string().c_str(),1);
#endif
    assert(settingsPath()==folder/"xr1-l-player/settings");
    auto check=[&](const char *name,bool explicitTest,std::vector<Input> inputs,bool expectOverlay,int seconds=1){
        script=std::move(inputs);nextInput=0;firstPoll=true;
        Options options;loadSettings(options);options.demo=true;options.fpsTest=explicitTest;options.seconds=seconds;options.screenshot=(folder/(std::string(name)+".bmp")).string();
        std::ostringstream output;auto *previous=std::cout.rdbuf(output.rdbuf());
        int result=app(options);std::cout.rdbuf(previous);std::cout<<output.str();assert(result==0);assert(nextInput==script.size());
        auto summary=output.str();auto at=summary.rfind("Frames received:");assert(at!=std::string::npos);
        unsigned long long received=0,shown=0;
        assert(std::sscanf(summary.c_str()+at,"Frames received: %llu / displayed: %llu",&received,&shown)==2);
        assert(received>0&&received==shown); // Includes stop/restart: counts must describe the same feed.
        auto *surface=SDL_LoadBMP(options.screenshot.c_str());assert(surface&&surface->format->BytesPerPixel==4);
        // This patch is red source video when hidden, darkened by the panel
        // when open. It avoids all text, buttons, and the animated test marker.
        int x=surface->w*700/1100,y=surface->h*104/650;Uint32 pixel=0;
        std::memcpy(&pixel,static_cast<Uint8*>(surface->pixels)+y*surface->pitch+x*4,4);
        Uint8 r,g,b;SDL_GetRGB(pixel,surface->format,&r,&g,&b);SDL_FreeSurface(surface);
        assert(expectOverlay?r<80:r>100);
        std::cout<<"PASS player FPS overlay: "<<name<<"\n";
    };
    check("normal-start-closed",false,{},false);
    check("explicit-test-open",true,{},true);
    check("F8-open",false,{{150,SDL_KEYDOWN,SDLK_F8}},true);
    check("F8-close",false,{{150,SDL_KEYDOWN,SDLK_F8},{350,SDL_KEYDOWN,SDLK_F8}},false);
    check("new-feed-closed",false,{{150,SDL_KEYDOWN,SDLK_F8},{350,SDL_KEYDOWN,SDLK_RETURN},{550,SDL_KEYDOWN,SDLK_RETURN}},false);
    check("renderer-reset-closed",false,{{250,SDL_RENDER_DEVICE_RESET}},false);
    check("volume-muted",false,{{150,SDL_KEYDOWN,SDLK_MINUS},{250,SDL_KEYDOWN,SDLK_MINUS},{350,SDL_KEYDOWN,SDLK_m}},false);
    Options saved;loadSettings(saved);assert(std::abs(saved.volume-.6f)<.0001f&&saved.muted);
    check("volume-restored",false,{},false);
    auto *restored=SDL_LoadBMP((folder/"volume-restored.bmp").string().c_str());assert(restored);
    auto pixelAt=[&](int x,int y){
        Uint32 pixel=0;x=x*restored->w/1100;y=y*restored->h/650;
        std::memcpy(&pixel,static_cast<Uint8*>(restored->pixels)+y*restored->pitch+x*4,4);
        SDL_Color color{};SDL_GetRGB(pixel,restored->format,&color.r,&color.g,&color.b);return color;
    };
    assert(pixelAt(515,606).b>200); // Real Audio volume puts the slider knob at 60%, not the old 70% default.
    int mutedTextPixels=0;for(int y=29;y<49;++y)for(int x=992;x<1068;++x){auto c=pixelAt(x,y);if(c.r>100&&c.g>100&&c.b>120)++mutedTextPixels;}
    assert(mutedTextPixels>10);SDL_FreeSurface(restored); // Real Audio mute flag draws the Muted indicator after relaunch.
    check("volume-unmuted",false,{{150,SDL_KEYDOWN,SDLK_m}},false);
    Options unmuted;loadSettings(unmuted);assert(std::abs(unmuted.volume-.6f)<.0001f&&!unmuted.muted);
    std::cout<<"PASS volume level and mute/unmute restored across actual player launches\n";
    auto temporary=settingsPath();
#ifdef _WIN32
    temporary+=".tmp."+std::to_string(GetCurrentProcessId());
#else
    temporary+=".tmp."+std::to_string(getpid());
#endif
    std::filesystem::create_directory(temporary); // Fail once; failed-save cleanup removes this blocker.
    check("volume-save-retry",false,{{150,SDL_KEYDOWN,SDLK_PLUS},{3000,SDL_USEREVENT}},false,4);
    Options retried;loadSettings(retried);assert(std::abs(retried.volume-.65f)<.0001f&&!retried.muted);
    auto report=folder/"xr1-l-player"/
#ifdef _WIN32
        "windows-fps-test.txt";
#else
        "fps-test.txt";
#endif
    std::filesystem::remove(report);
#ifdef _WIN32
    std::filesystem::create_directory(report); // Cannot write a report over a directory.
#else
    std::filesystem::create_symlink("/dev/full",report); // Writes fail only when the stream is flushed.
#endif
    std::ostringstream log;auto *console=std::cout.rdbuf(log.rdbuf());
    check("report-write-failure",true,{},true);
    std::cout.rdbuf(console);assert(log.str().find("FPS report could not be saved:")!=std::string::npos);
    std::cout<<"PASS FPS report write/flush failure detected\n";
    std::filesystem::remove_all(folder);
}
