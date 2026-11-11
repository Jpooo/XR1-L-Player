#include <SDL.h>
#include <cassert>
static SDL_Window *observedWindow=nullptr;
static int stage=0;static Uint32 started=0;static bool acted=false;
static SDL_Window *observeWindow(const char *title,int x,int y,int w,int h,Uint32 flags){
    if(stage>0&&stage<4)assert(w==1280&&h==760);
    if(stage==4)assert(w<=1920&&h<=1080);
    observedWindow=SDL_CreateWindow(title,x,y,w,h,flags|SDL_WINDOW_HIDDEN);return observedWindow;
}
static int displayBounds(int,SDL_Rect *bounds){*bounds={0,0,1920,1080};return 0;}
static int windowPoll(SDL_Event *event){
    if(!started)started=SDL_GetTicks();
    if(!acted&&SDL_GetTicks()-started>200){
        acted=true;
        if(stage==0)SDL_SetWindowSize(observedWindow,1280,760);
        if(stage==1||stage==2){
            assert(bool(SDL_GetWindowFlags(observedWindow)&SDL_WINDOW_FULLSCREEN_DESKTOP)==(stage==2));
            *event={};event->type=SDL_KEYDOWN;event->key.keysym.sym=SDLK_F11;return 1;
        }
    }
    return SDL_PollEvent(event);
}
#define SDL_CreateWindow observeWindow
#define SDL_GetDisplayUsableBounds displayBounds
#define SDL_PollEvent windowPoll
#define main window_application_main
#include "../src/main.cpp"
#undef main
#undef SDL_CreateWindow
#undef SDL_GetDisplayUsableBounds
#undef SDL_PollEvent
int main(){
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
#ifndef _WIN32
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);auto id=getpid();
#else
    auto id=GetCurrentProcessId();
#endif
    auto folder=std::filesystem::temp_directory_path()/("xr1-window-"+std::to_string(id));std::filesystem::create_directories(folder);SDL_setenv("XDG_CONFIG_HOME",folder.string().c_str(),1);
    for(stage=0;stage<4;++stage){
        started=0;acted=false;Options options;loadSettings(options);options.seconds=1;
        if(stage>0)assert(options.windowWidth==1280&&options.windowHeight==760);
        assert(options.fullscreen==(stage==2));assert(app(options)==0);
    }
    stage=4;started=0;acted=false;assert(updateSettings({{"window_width","8192"},{"window_height","8192"}}));Options oversized;loadSettings(oversized);oversized.seconds=1;assert(app(oversized)==0);Options bounded;loadSettings(bounded);assert(bounded.windowWidth<=1920&&bounded.windowHeight<=1080);
    assert(updateSettings({{"window_width","1280"},{"window_height","760"}}));
    Options saved;loadSettings(saved);assert(!saved.fullscreen&&saved.windowWidth==1280&&saved.windowHeight==760);
    std::filesystem::remove_all(folder);std::cout<<"PASS resize/relaunch, fullscreen restore, leaving fullscreen restores windowed size\n";
}
