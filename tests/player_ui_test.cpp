// Drive the real player with timed keyboard/mouse input, without hardware.
#include <SDL.h>
#include <SDL_ttf.h>
#include <set>
#include <string>
static int injectedPoll(SDL_Event *);
static SDL_Window *measuredWindow(const char *,int,int,int,int,Uint32);
static SDL_Surface *measuredText(TTF_Font *,const char *,SDL_Color);
#define SDL_PollEvent injectedPoll
#define SDL_CreateWindow measuredWindow
#define TTF_RenderUTF8_Blended measuredText
#define main player_application_main
#include "../src/main.cpp"
#undef main
#undef SDL_PollEvent
#undef SDL_CreateWindow
#undef TTF_RenderUTF8_Blended
#include <cassert>
struct Input {Uint32 at,type;SDL_Keycode key=0;int x=0,y=0;};
static std::vector<Input> script;
static std::set<std::string> labels;
static Uint32 pollBegan=0;static size_t nextInput=0;static bool firstPoll=true;
static int targetW=1100,targetH=650;
static SDL_Surface *measuredText(TTF_Font *font,const char *text,SDL_Color color){labels.insert(text);return TTF_RenderUTF8_Blended(font,text,color);}
static SDL_Window *measuredWindow(const char *title,int x,int y,int,int,Uint32 flags){return SDL_CreateWindow(title,x,y,targetW,targetH,flags);}
static int injectedPoll(SDL_Event *event){
    if(firstPoll){pollBegan=SDL_GetTicks();firstPoll=false;}
    if(SDL_PollEvent(event))return 1;
    if(nextInput==script.size()||SDL_GetTicks()-pollBegan<script[nextInput].at)return 0;
    const auto &input=script[nextInput++];*event={};event->type=input.type;
    if(input.type==SDL_KEYDOWN)event->key.keysym.sym=input.key;
    if(input.type==SDL_MOUSEBUTTONDOWN||input.type==SDL_MOUSEBUTTONUP){event->button.button=SDL_BUTTON_LEFT;event->button.x=input.x;event->button.y=input.y;}
    if(input.type==SDL_USEREVENT){Options saved;loadSettings(saved);assert(std::abs(saved.volume-.65f)<.0001f&&!saved.muted);}
    return 1;
}
int main(){
    [[maybe_unused]] CapturePlatform platform;
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
#ifndef _WIN32
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);auto id=getpid();
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
    auto key=[](Uint32 at,SDL_Keycode k){return Input{at,SDL_KEYDOWN,k};};
    auto click=[](Uint32 at,int x,int y){return Input{at,SDL_MOUSEBUTTONDOWN,0,x,y};};
    auto check=[&](const char *name,bool demo,std::vector<Input> inputs,const char *expected,bool feed,int seconds=1){
        script=std::move(inputs);nextInput=0;firstPoll=true;labels.clear();
        Options options;loadSettings(options);options.demo=demo;options.seconds=seconds;options.screenshot=(folder/(std::string(name)+".bmp")).string();
        std::ostringstream output;auto *previous=std::cout.rdbuf(output.rdbuf());int result=app(options);std::cout.rdbuf(previous);std::cout<<output.str();
        assert(result==0&&nextInput==script.size());if(expected)assert(labels.count(expected));
        for(const auto &text:labels)assert(text.find("FPS test")==std::string::npos&&text.find("Xbox")==std::string::npos&&text.find("PlayStation")==std::string::npos&&text.find("Nintendo")==std::string::npos);
        auto summary=output.str();auto at=summary.rfind("Frames received:");assert(at!=std::string::npos);unsigned long long received=0,shown=0;
        assert(std::sscanf(summary.c_str()+at,"Frames received: %llu / displayed: %llu",&received,&shown)==2);
        if(demo)assert(received>0);
        if(feed)assert(shown>0);
        auto *surface=SDL_LoadBMP(options.screenshot.c_str());assert(surface&&surface->w==targetW&&surface->h==targetH);
        if(feed&&targetW==1100&&targetH==650){
            int x=700,y=104;Uint32 pixel=0;std::memcpy(&pixel,static_cast<Uint8*>(surface->pixels)+y*surface->pitch+x*4,4);
            Uint8 r,g,b;SDL_GetRGB(pixel,surface->format,&r,&g,&b);assert(r>100); // Clean video, including after the removed shortcut.
        }
        SDL_FreeSurface(surface);
        assert(!std::filesystem::exists(folder/"xr1-l-player/fps-test.txt")&&!std::filesystem::exists(folder/"xr1-l-player/windows-fps-test.txt"));
        std::cout<<"PASS real player: "<<name<<"\n";
    };
    check("main-menu",false,{},"PLAY",false);assert(labels.count("SETTINGS")&&labels.count("PREFERENCES"));
    check("mouse-settings",false,{click(100,150,350),click(250,180,330)},"yuyv",false);assert(labels.count("VIDEO SETTINGS")&&labels.count("AUDIO SETTINGS"));
    check("keyboard-settings",false,{key(100,SDLK_DOWN),key(200,SDLK_RETURN)},"VIDEO SETTINGS",false);
    check("preferences-all-colors",false,{key(100,SDLK_DOWN),key(150,SDLK_DOWN),key(200,SDLK_RETURN),click(300,800,220),click(400,100,220),click(500,430,220),click(600,100,410)},"Electric Blue",false);
    Options appearance;loadSettings(appearance);assert(appearance.colorTheme==ui::ConsoleColor::PlayStation&&appearance.reduceMotion);
    check("preferences-restored",false,{click(100,150,460)},"OFF",false);
    targetW=900;targetH=560;check("minimum-window",false,{click(100,150,460)},"COLOR PREFERENCE",false);
    targetW=1600;targetH=900;check("large-window",false,{},"PLAY",false);
    targetW=1100;targetH=650;
    check("feed-start-clean",true,{},nullptr,true);
    check("removed-shortcut-inert",true,{key(200,SDLK_F8)},nullptr,true);
    check("stop-restart-play",true,{key(150,SDLK_RETURN),key(350,SDLK_RETURN)},"PLAY",true);
    check("settings-resume",true,{key(100,SDLK_TAB),key(350,SDLK_TAB)},"AUDIO SETTINGS",true);
    check("preferences-while-playing",true,{key(100,SDLK_TAB),key(200,SDLK_ESCAPE),click(300,150,460),click(400,430,220),key(600,SDLK_ESCAPE),click(700,150,240)},"COLOR PREFERENCE",true);
    check("renderer-reset",true,{{250,SDL_RENDER_DEVICE_RESET}},nullptr,true);
    check("volume-muted",true,{key(150,SDLK_MINUS),key(250,SDLK_MINUS),key(350,SDLK_m)},nullptr,true);
    Options saved;loadSettings(saved);assert(std::abs(saved.volume-.6f)<.0001f&&saved.muted);
    check("volume-restored",true,{},"Muted",true);
    check("volume-unmuted",true,{key(150,SDLK_m)},nullptr,true);
    Options unmuted;loadSettings(unmuted);assert(std::abs(unmuted.volume-.6f)<.0001f&&!unmuted.muted);
    auto temporary=settingsPath();temporary+=".tmp."+std::to_string(id);std::filesystem::create_directory(temporary);
    check("volume-save-retry",true,{key(150,SDLK_PLUS),{3000,SDL_USEREVENT}},nullptr,true,4);
    char arg0[]="xr1-l-player",arg1[]="--fps-test";char *argv[]={arg0,arg1};bool removed=false;
    try{parse(2,argv);}catch(const std::runtime_error &){removed=true;}assert(removed);
    if(std::getenv("XR1_KEEP_UI_TEST_OUTPUT"))std::cout<<"UI screenshots: "<<folder.string()<<"\n";else std::filesystem::remove_all(folder);
}
