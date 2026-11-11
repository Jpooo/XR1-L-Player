// Verify preference behavior and the real Settings action, without hardware.
#include "capture_fixture.hpp"
#undef Capture
#include <cassert>
#include <fstream>

class SharpFixtureCapture : public fixture::Capture {
public:
    using fixture::Capture::Capture;
    inline static int starts=0;
    void start(const std::string &path,const std::string &format,unsigned buffers){
        ++starts;fixture::Capture::start(path,format,buffers);
    }
};
static int sharpPoll(SDL_Event *event);
#define Capture SharpFixtureCapture
#define SDL_PollEvent sharpPoll
#define main sharp_preference_application_main
#include "../src/main.cpp"
#undef main
#undef Capture
#undef videoDevices
#undef SDL_GetNumAudioDevices
#undef SDL_GetAudioDeviceName
#undef SDL_PollEvent

static Uint32 began=0;
static size_t nextAction=0;
static int checkedActions=0;
struct SharpAction {Uint32 at;SDL_Keycode key;bool inspect;bool expected;};
static const SharpAction actions[]={
    {100,SDLK_TAB,false,false},       // Playing -> Settings, focus starts at0.
    {250,SDLK_DOWN,false,false},      // Video format.
    {350,SDLK_DOWN,false,false},      // Sharper fullscreen, new focus index2.
    {450,SDLK_RETURN,false,false},    // Disable without stopping the feed.
    {700,0,true,false},
    {800,SDLK_RIGHT,false,false},     // Toggle with the alternate keyboard path.
    {1100,0,true,true}
};
static int sharpPoll(SDL_Event *event){
    if(!began)began=SDL_GetTicks();
    if(SDL_PollEvent(event))return 1;
    if(nextAction==std::size(actions)||SDL_GetTicks()-began<actions[nextAction].at)return 0;
    auto action=actions[nextAction++];
    if(action.inspect){
        Options saved;loadSettings(saved);assert(saved.sharpFullscreen==action.expected);
        assert(saved.format=="nv12"&&saved.video=="fixture://xr1-lite"&&saved.audio=="XR1 Lite fixture HDMI");
        assert(std::abs(saved.volume-.42f)<.0001f&&saved.muted&&saved.colorTheme==ui::ConsoleColor::PlayStation);
        assert(SharpFixtureCapture::starts==1);++checkedActions;return 0;
    }
    *event={};event->type=SDL_KEYDOWN;event->key.keysym.sym=action.key;return 1;
}

int main(){
    [[maybe_unused]] CapturePlatform platform;
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
#ifndef _WIN32
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);auto id=getpid();
#else
    auto id=GetCurrentProcessId();
#endif
    auto folder=std::filesystem::temp_directory_path()/("xr1-sharp-pref-"+std::to_string(id)+"-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);SDL_setenv("XDG_CONFIG_HOME",folder.string().c_str(),1);
    Options defaults;assert(defaults.sharpFullscreen);
    assert(saveSettings("fixture://xr1-lite","XR1 Lite fixture HDMI","","nv12",ui::ConsoleColor::PlayStation,.42f,true));
    assert(updateSettings({{"window_width","1280"},{"window_height","760"},{"fullscreen","false"},{"custom","keep this field"}}));
    assert(saveSharpPreference(false));
    Options disabled;loadSettings(disabled);assert(!disabled.sharpFullscreen&&disabled.format=="nv12"&&disabled.windowWidth==1280&&disabled.windowHeight==760&&!disabled.fullscreen);
    assert(disabled.colorTheme==ui::ConsoleColor::PlayStation&&std::abs(disabled.volume-.42f)<.0001f&&disabled.muted);
    assert(saveColorPreference(ui::ConsoleColor::Xbox));assert(saveVolumePreference(.6f,false));
    Options merged;loadSettings(merged);assert(!merged.sharpFullscreen&&merged.colorTheme==ui::ConsoleColor::Xbox&&std::abs(merged.volume-.6f)<.0001f&&!merged.muted);
    assert(saveSharpPreference(true));Options enabled;loadSettings(enabled);assert(enabled.sharpFullscreen);
    for(const char *bad:{"nonsense","0","TRUE",""}){
        assert(updateSettings({{"sharp_fullscreen",bad}}));Options invalid;loadSettings(invalid);assert(invalid.sharpFullscreen);
    }
    assert(saveSharpPreference(false));
    auto path=settingsPath();std::ifstream before(path);std::string original((std::istreambuf_iterator<char>(before)),{});before.close();
    assert(original.find("custom \"keep this field\"")!=std::string::npos);
    auto temporary=path;temporary+=".tmp."+std::to_string(id);std::filesystem::create_directory(temporary);
    assert(!saveSharpPreference(true));std::ifstream after(path);std::string retained((std::istreambuf_iterator<char>(after)),{});after.close();assert(retained==original);
    std::filesystem::remove(temporary);
    // Restore the exact playback setup before checking the in-app action.
    assert(saveSettings("fixture://xr1-lite","XR1 Lite fixture HDMI","","nv12",ui::ConsoleColor::PlayStation,.42f,true));
    assert(saveSharpPreference(true));fixture::enabled=true;
    Options run;loadSettings(run);run.autostart=true;run.seconds=2;assert(app(run)==0);
    assert(checkedActions==2&&nextAction==std::size(actions)&&SharpFixtureCapture::starts==1);
    Options final;loadSettings(final);assert(final.sharpFullscreen);
    std::filesystem::remove_all(folder);
    std::cout<<"PASS sharp preference defaults/validation, merged settings, failed-write preservation, live Settings focus and no capture restart\n";
}
