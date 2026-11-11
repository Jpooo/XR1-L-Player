// Simulate device hotplug at the native API boundary and exercise the real UI.
#include "../src/video.hpp"
#include "../src/audio.hpp"
#include <SDL_ttf.h>
#include <cassert>
#include <set>
static std::vector<VideoDevice> fixtureVideoDevices();
static int fixtureAudioCount(int);static const char *fixtureAudioName(int,int);
static int fixturePoll(SDL_Event *);static SDL_Surface *fixtureText(TTF_Font *,const char *,SDL_Color);
static SDL_Window *fixtureWindow(const char *,int,int,int,int,Uint32);
struct Open {std::string path,format,input,output;float volume=0;bool muted=false,fullscreen=false;};
static std::vector<Open> opens;static std::set<std::string> labels;
static SDL_Window *testWindow=nullptr;static Uint32 began=0;static int scenario=0;static bool videoPresent=true,inputPresent=true,outputPresent=true,lost=false;static int action=0;
static const VideoDevice selected{"device://vid_3842&pid_310a#unit-a","EVGA XR1 Lite","3842:310a"};
class FixtureCapture {
    SharedVideo &shared;Frame frame;double last=0;
public:
    static FixtureCapture *active;
    explicit FixtureCapture(SharedVideo &value):shared(value){}
    void start(const std::string &path,const std::string &format,unsigned){
        opens.push_back({path,format,"","",0,false,bool(SDL_GetWindowFlags(testWindow)&SDL_WINDOW_FULLSCREEN_DESKTOP)});
        if(scenario==3&&opens.size()==2)throw std::runtime_error("Cannot open camera: busy");
        assert(videoPresent&&path==selected.path);active=this;last=0;
        std::lock_guard<std::mutex> lock(shared.mutex);shared.error.clear();shared.dirty=false;shared.captured=0;shared.skipped=0;shared.corrupt=0;shared.mode="fixture 1080p60";
    }
    void stop(){if(active==this)active=nullptr;}
    void tick(){if(!videoPresent||monotime()-last<.05)return;last=monotime();frame.pixel=SDL_PIXELFORMAT_RGB24;frame.pitch=5760;frame.data.assign(size_t(5760)*1080,170);frame.received=last;shared.publish(frame);}
    void fail(){shared.fail("Capture card disconnected or stopped streaming.");}
#ifdef _WIN32
    WinCaptureSnapshot snapshot(){WinCaptureSnapshot value;value.samples=shared.captured.load();value.fps=value.samples?20:0;return value;}
#endif
};
FixtureCapture *FixtureCapture::active=nullptr;
class FixtureAudio {
public:
    AudioRing ring;std::atomic<float> volume{.7f};std::atomic<bool> muted{false};std::string mode="fixture audio";
    explicit FixtureAudio(int){}bool ownsDevice(Uint32 id)const{return id==123;}
    void start(const char *input,const char *output,bool demo){assert(!demo&&inputPresent&&outputPresent&&input&&output);opens.back().input=input;opens.back().output=output;opens.back().volume=volume;opens.back().muted=muted;}
    void stop(){}
};
#define Capture FixtureCapture
#define Audio FixtureAudio
#define videoDevices fixtureVideoDevices
#define SDL_GetNumAudioDevices fixtureAudioCount
#define SDL_GetAudioDeviceName fixtureAudioName
#define SDL_PollEvent fixturePoll
#define SDL_CreateWindow fixtureWindow
#define TTF_RenderUTF8_Blended fixtureText
#define main recovery_application_main
#include "../src/main.cpp"
#undef main
#undef Capture
#undef Audio
#undef videoDevices
#undef SDL_GetNumAudioDevices
#undef SDL_GetAudioDeviceName
#undef SDL_PollEvent
#undef SDL_CreateWindow
#undef TTF_RenderUTF8_Blended
static std::vector<VideoDevice> fixtureVideoDevices(){std::vector<VideoDevice> result{{"device://webcam","Unrelated webcam","0000:1111"}};if(videoPresent)result.push_back(selected);return result;}
static int fixtureAudioCount(int capture){return 1+(capture?inputPresent:outputPresent);}
static const char *fixtureAudioName(int index,int capture){if(index==0)return capture?"Other microphone":"Speakers";return capture?"XR1 Lite HDMI":"Headphones";}
static SDL_Surface *fixtureText(TTF_Font *font,const char *text,SDL_Color color){labels.insert(text);return TTF_RenderUTF8_Blended(font,text,color);}
static SDL_Window *fixtureWindow(const char *title,int x,int y,int w,int h,Uint32 flags){testWindow=SDL_CreateWindow(title,x,y,w,h,flags);return testWindow;}
static int fixturePoll(SDL_Event *event){
    if(!began)began=SDL_GetTicks();
    Uint32 time=SDL_GetTicks()-began;
    if(time>=250&&!lost){
        lost=true;assert(FixtureCapture::active);
        if(scenario==5){inputPresent=false;*event={};event->type=SDL_AUDIODEVICEREMOVED;event->adevice.which=123;event->adevice.iscapture=1;return 1;}
        videoPresent=inputPresent=outputPresent=false;FixtureCapture::active->fail();
    }
    if(scenario==0){if(time>=1400)videoPresent=true;if(time>=2400)inputPresent=true;if(time>=3400)outputPresent=true;}
    else if(time>=900)videoPresent=inputPresent=outputPresent=true;
    if(FixtureCapture::active)FixtureCapture::active->tick();
    if(SDL_PollEvent(event))return 1;
    if((scenario==4||scenario==6)&&time>=450&&!action){action=true;*event={};event->type=SDL_MOUSEBUTTONDOWN;event->button.button=SDL_BUTTON_LEFT;event->button.x=150;event->button.y=350;return 1;}
    if(scenario==6&&time>=650&&action&&action!=2){action=2;*event={};event->type=SDL_MOUSEBUTTONDOWN;event->button.button=SDL_BUTTON_LEFT;event->button.x=180;event->button.y=330;return 1;}
    if((scenario==1||scenario==2)&&time>=600&&!action){action=true;*event={};event->type=scenario==2?SDL_QUIT:SDL_KEYDOWN;event->key.keysym.sym=SDLK_ESCAPE;return 1;}
    return 0;
}
int main(){
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
#ifndef _WIN32
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);auto id=getpid();
#else
    auto id=GetCurrentProcessId();
#endif
    auto folder=std::filesystem::temp_directory_path()/("xr1-recovery-"+std::to_string(id));std::filesystem::create_directories(folder);
#ifdef _WIN32
    _putenv_s("XDG_CONFIG_HOME",folder.string().c_str());
#else
    SDL_setenv("XDG_CONFIG_HOME",folder.string().c_str(),1);
#endif
    for(scenario=0;scenario<7;++scenario){
        opens.clear();labels.clear();began=0;lost=false;action=0;videoPresent=inputPresent=outputPresent=true;
        Options options;options.autostart=true;options.video=selected.path;options.audio="XR1 Lite HDMI";options.output="Headphones";options.format="nv12";options.videoSet=options.audioSet=options.outputSet=true;options.fullscreen=true;options.volume=.42f;options.muted=true;options.seconds=scenario==0?6:scenario==3?5:3;options.screenshot=(folder/("scenario-"+std::to_string(scenario)+".bmp")).string();
        assert(app(options)==0);
        if(scenario==0||scenario==3||scenario==4||scenario==5){
            assert(opens.size()==(scenario==3?3u:2u));auto &last=opens.back();assert(last.path==selected.path&&last.format=="nv12"&&last.input=="XR1 Lite HDMI"&&last.output=="Headphones");assert(std::abs(last.volume-.42f)<.001f&&last.muted&&last.fullscreen);
        }else assert(opens.size()==1);
        if(scenario==4){
            auto *image=SDL_LoadBMP(options.screenshot.c_str());assert(image);float scale=std::min(image->w/1100.f,image->h/650.f);
            int x=int((image->w-1100*scale)/2+100*scale),y=int((image->h-650*scale)/2+300*scale);Uint32 pixel=0;std::memcpy(&pixel,static_cast<Uint8*>(image->pixels)+y*image->pitch+x*4,4);
            Uint8 r,g,b;SDL_GetRGB(pixel,image->format,&r,&g,&b);assert(b<80);SDL_FreeSurface(image); // Menu stays open after reconnect.
        }
        if(scenario==6)assert(labels.count("uyvy"));
        assert(labels.count("Reconnecting")&&labels.count("CANCEL"));
        if(scenario==0){assert(labels.count(connection::waiting(connection::Match::InputMissing)));assert(labels.count(connection::waiting(connection::Match::OutputMissing)));}
        std::cout<<"PASS simulated player reconnect scenario "<<scenario<<"\n";
    }
    std::filesystem::remove_all(folder);
}
