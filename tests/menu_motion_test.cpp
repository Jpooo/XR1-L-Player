#include <SDL.h>
static void measuredPresent(SDL_Renderer *);
static int measuredIcon(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const SDL_FRect *,double,const SDL_FPoint *,SDL_RendererFlip);
#define SDL_RenderPresent measuredPresent
#define SDL_RenderCopyExF measuredIcon
#define main motion_application_main
#include "../src/main.cpp"
#undef main
#undef SDL_RenderPresent
#undef SDL_RenderCopyExF
#include <cassert>
static std::vector<double> paintTimes;
static std::vector<float> positions;
static std::vector<double> angles;
static void measuredPresent(SDL_Renderer *renderer){paintTimes.push_back(monotime());SDL_RenderPresent(renderer);}
static int measuredIcon(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_FRect *dst,double angle,const SDL_FPoint *center,SDL_RendererFlip flip){
    if(dst&&std::abs(dst->x-780)<.01f&&std::abs(dst->w-212)<.01f){positions.push_back(dst->y);angles.push_back(angle);}
    return SDL_RenderCopyExF(r,t,src,dst,angle,center,flip);
}
int main(){
    assert(ui::menuFrameRate(0,false,false)==60&&ui::menuFrameRate(144,false,false)==144);
    assert(ui::menuFrameRate(59,false,false)==60&&ui::menuFrameRate(360,false,false)==240);
    assert(ui::menuFrameRate(144,true,false)==20&&ui::menuFrameRate(144,false,true)==60);
    ui::MenuFrameClock clock;assert(clock.due(0));clock.painted(0);
    for(int i=1;i<=600;++i){clock.painted(clock.deadline(0)+.0004);assert(std::abs(clock.deadline(0)-(i+1)/60.0)<1e-8);}
    clock.painted(30);assert(!clock.due(30)&&clock.deadline(0)<=30+1.0/60+1e-8);
    clock.rate(144);assert(clock.due(30));clock.painted(30);assert(std::abs(clock.deadline(0)-(30+1.0/144))<1e-8);
    assert(ui::controllerBob(.1,false)>0&&ui::controllerBob(.1,false)<1&&ui::controllerBob(.1,true)==0);
    std::cout<<"PASS refresh-aware menu rate, fixed-phase deadlines, stall recovery and fractional motion\n";
    [[maybe_unused]] CapturePlatform platform;
    if(!std::getenv("XR1_MOTION_GPU")){
        SDL_setenv("SDL_RENDER_DRIVER","software",1);
#ifndef _WIN32
        SDL_setenv("SDL_VIDEODRIVER","dummy",1);
#endif
    }
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    for(bool reduced:{false,true}){
        paintTimes.clear();positions.clear();angles.clear();
        Options options;options.seconds=2;options.reduceMotion=reduced;assert(app(options)==0);assert(positions.size()>2);
        bool fractional=false;for(auto y:positions){if(std::abs(y-std::round(y))>.001f)fractional=true;if(reduced)assert(std::abs(y-(244+ui::menuOffset))<.001f);}
        if(reduced){assert(!fractional);for(auto angle:angles)assert(angle==-10);}
        else assert(fractional);
        // Report observed timing; do not turn virtual-host speed into a hard
        // performance assertion or a claim about capture-card throughput.
        std::vector<double> intervals;for(size_t i=1;i<paintTimes.size();++i)if(paintTimes[i]-paintTimes.front()>.3)intervals.push_back((paintTimes[i]-paintTimes[i-1])*1000);
        assert(!intervals.empty());std::sort(intervals.begin(),intervals.end());
        std::cout<<"PASS actual menu "<<(reduced?"motion off":"fractional movement")<<" / median paint interval "<<intervals[intervals.size()/2]<<" ms\n";
    }
}
