#pragma once
#include "core.hpp"
#include <windows.h>
#include <avrt.h>
#include <array>
// Reuse two GPU destinations without queuing extra video frames. Updating the
// texture currently sampled by the GPU can serialize the render thread.
class WindowsVideoUpload {
    SDL_Renderer *renderer=nullptr;
    std::array<SDL_Texture*,2> textures{};
    Uint32 format=0;
    unsigned next=0,slots=0;
public:
    WindowsVideoUpload()=default;
    WindowsVideoUpload(const WindowsVideoUpload&)=delete;
    WindowsVideoUpload &operator=(const WindowsVideoUpload&)=delete;
    ~WindowsVideoUpload(){reset();}
    void reset(){for(auto *t:textures)if(t)SDL_DestroyTexture(t);textures={};format=0;next=slots=0;renderer=nullptr;}
    unsigned destinations()const{return slots;}
    SDL_Texture *upload(SDL_Renderer *r,const Frame &frame){
        if(renderer!=r||format!=frame.pixel||!slots){
            reset();renderer=r;format=frame.pixel;
            for(unsigned i=0;i<textures.size();++i){
                textures[i]=SDL_CreateTexture(r,format,SDL_TEXTUREACCESS_STREAMING,1920,1080);
                if(!textures[i])break; // One texture remains a valid low-memory fallback.
                SDL_SetTextureScaleMode(textures[i],SDL_ScaleModeLinear);++slots;
            }
        }
        if(!slots)return nullptr;
        auto *texture=textures[next];
        if(SDL_UpdateTexture(texture,nullptr,frame.data.data(),frame.pitch)<0)return nullptr;
        next=(next+1)%slots;
        return texture;
    }
};
// MMCSS coordinates playback work with the audio/compositor threads. This is
// scoped to active preview on this thread; failure keeps standard scheduling.
class WindowsPlaybackScheduling {
    HANDLE task=nullptr;
public:
    WindowsPlaybackScheduling()=default;
    WindowsPlaybackScheduling(const WindowsPlaybackScheduling&)=delete;
    WindowsPlaybackScheduling &operator=(const WindowsPlaybackScheduling&)=delete;
    ~WindowsPlaybackScheduling(){stop();}
    void start(){if(task)return;DWORD index=0;task=AvSetMmThreadCharacteristicsW(L"Games",&index);if(task&&!AvSetMmThreadPriority(task,AVRT_PRIORITY_HIGH)){AvRevertMmThreadCharacteristics(task);task=nullptr;}}
    void stop(){if(task){AvRevertMmThreadCharacteristics(task);task=nullptr;}}
    const char *mode()const{return task?"MMCSS Games / high":"standard (MMCSS unavailable)";}
};
