// Inject real SDL input events into the app; check its actual video-texture mode.
#include <SDL.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
struct Action {Uint32 at,type;int x,y;SDL_Keycode key=0;Uint8 clicks=1;};
static Uint32 firstPollTick=0;
static bool firstPollSeen=false;
static const Action actions[]={
    {200,SDL_MOUSEMOTION,450,606},
    {300,SDL_MOUSEBUTTONDOWN,450,606},
    {400,SDL_MOUSEMOTION,565,606},
    {450,SDL_MOUSEBUTTONUP,565,606},
    {600,SDL_MOUSEBUTTONDOWN,350,606},
    {650,SDL_MOUSEBUTTONUP,350,606},
    {800,SDL_MOUSEBUTTONDOWN,900,606},
    {1000,SDL_KEYDOWN,0,0,SDLK_F11},
    {1200,SDL_MOUSEBUTTONDOWN,500,300,0,2},
    {1400,SDL_KEYDOWN,0,0,SDLK_ESCAPE},
    {1600,SDL_KEYDOWN,0,0,SDLK_m},
    {1800,SDL_MOUSEBUTTONDOWN,240,606},
    {2000,SDL_MOUSEBUTTONDOWN,780,418},{2100,SDL_MOUSEBUTTONUP,780,418},
    {2300,SDL_MOUSEBUTTONDOWN,940,528},
    {2600,SDL_KEYDOWN,0,0,SDLK_TAB},
    {2800,SDL_MOUSEBUTTONDOWN,450,528},{3000,SDL_MOUSEBUTTONDOWN,450,528},
    {3150,SDL_KEYDOWN,0,0,SDLK_ESCAPE},{3300,SDL_MOUSEBUTTONDOWN,150,460},
    {3400,SDL_MOUSEBUTTONDOWN,100,410},{3550,SDL_MOUSEBUTTONDOWN,100,410},
    {3700,SDL_KEYDOWN,0,0,SDLK_ESCAPE}
};
static const Action appearanceActions[]={
    {100,SDL_KEYDOWN,0,0,SDLK_TAB},{200,SDL_KEYDOWN,0,0,SDLK_ESCAPE},
    {300,SDL_MOUSEBUTTONDOWN,150,460},
    {400,SDL_MOUSEBUTTONDOWN,430,220},{600,SDL_MOUSEBUTTONDOWN,800,220},
    {800,SDL_MOUSEBUTTONDOWN,100,220},{1000,SDL_MOUSEBUTTONDOWN,430,220},
    {1200,SDL_MOUSEBUTTONDOWN,100,410},{1400,SDL_MOUSEBUTTONDOWN,100,410},
    {1600,SDL_KEYDOWN,0,0,SDLK_ESCAPE},{1800,SDL_MOUSEBUTTONDOWN,150,240},
    {2000,SDL_KEYDOWN,0,0,SDLK_F8},
    {2200,SDL_KEYDOWN,0,0,SDLK_TAB},{2400,SDL_KEYDOWN,0,0,SDLK_TAB},
    {2600,SDL_KEYDOWN,0,0,SDLK_TAB},{2800,SDL_KEYDOWN,0,0,SDLK_ESCAPE},
    {3000,SDL_MOUSEBUTTONDOWN,150,460}
};
static const Action previewActions[]={{100,SDL_MOUSEBUTTONDOWN,100,606}};
extern "C" int SDL_PollEvent(SDL_Event *event) {
    static auto real=reinterpret_cast<int(*)(SDL_Event*)>(dlsym(RTLD_NEXT,"SDL_PollEvent"));
    if(!firstPollSeen){firstPollTick=SDL_GetTicks();firstPollSeen=true;}
    static bool resized=false;
    if(!resized&&std::getenv("XR1_TEST_SMALL")){if(auto *window=SDL_GetWindowFromID(1)){SDL_SetWindowSize(window,900,680);resized=true;}}
    if(real(event))return 1;
    static Uint32 began=SDL_GetTicks();static size_t next=0;
    bool appearance=std::getenv("XR1_TEST_APPEARANCE")!=nullptr;
    const auto *script=appearance?appearanceActions:actions;
    size_t length=appearance?sizeof(appearanceActions)/sizeof(appearanceActions[0]):sizeof(actions)/sizeof(actions[0]);
    if(std::getenv("XR1_TEST_PREVIEW")){script=previewActions;length=sizeof(previewActions)/sizeof(previewActions[0]);}
    if(next>=length||SDL_GetTicks()-began<script[next].at)return 0;
    const auto &a=script[next++];*event={};event->type=a.type;
    if(a.type==SDL_KEYDOWN)event->key.keysym.sym=a.key;
    else if(a.type==SDL_MOUSEMOTION){event->motion.x=a.x;event->motion.y=a.y;}
    else {event->button.button=SDL_BUTTON_LEFT;event->button.clicks=a.clicks;event->button.x=a.x;event->button.y=a.y;}
    return 1;
}
extern "C" int SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_Rect *dst) {
    static auto real=reinterpret_cast<int(*)(SDL_Renderer*,SDL_Texture*,const SDL_Rect*,const SDL_Rect*)>(dlsym(RTLD_NEXT,"SDL_RenderCopy"));
    int w=0,h=0;SDL_QueryTexture(t,nullptr,nullptr,&w,&h);
    if(w==1920&&h==1080){SDL_ScaleMode mode;SDL_GetTextureScaleMode(t,&mode);if(mode!=SDL_ScaleModeLinear){std::fprintf(stderr,"FAIL: video is not smoothly filtered\n");std::abort();}}
    return real(r,t,src,dst);
}

extern "C" void SDL_CloseAudioDevice(SDL_AudioDeviceID device){
    static auto real=reinterpret_cast<void(*)(SDL_AudioDeviceID)>(dlsym(RTLD_NEXT,"SDL_CloseAudioDevice"));
    if(std::getenv("XR1_TEST_APPEARANCE")&&device&&firstPollSeen&&SDL_GetTicks()-firstPollTick<3600){
        std::fprintf(stderr,"FAIL: appearance controls stopped audio playback early\n");std::abort();
    }
    real(device);
}
