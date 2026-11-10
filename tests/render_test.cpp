#include "../src/core.hpp"
#include <iostream>
int main() {
    if(SDL_Init(SDL_INIT_VIDEO)<0) {std::cerr<<SDL_GetError();return 1;}
    auto *w=SDL_CreateWindow("YUV texture test",0,0,640,360,SDL_WINDOW_HIDDEN);
    auto *r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED);if(!r) {std::cerr<<SDL_GetError();return 1;}
    for(auto format:{SDL_PIXELFORMAT_YUY2,SDL_PIXELFORMAT_NV12,SDL_PIXELFORMAT_IYUV,SDL_PIXELFORMAT_YV12,SDL_PIXELFORMAT_UYVY}) {
        bool planar=format==SDL_PIXELFORMAT_NV12||format==SDL_PIXELFORMAT_IYUV||format==SDL_PIXELFORMAT_YV12;
        int pitch=planar?1984:3904;
        std::vector<uint8_t> data(static_cast<size_t>(pitch)*(planar?1620:1080),128);
        auto *t=SDL_CreateTexture(r,format,SDL_TEXTUREACCESS_STREAMING,1920,1080);
        if(t)SDL_SetTextureScaleMode(t,SDL_ScaleModeLinear);
        if(!t||SDL_UpdateTexture(t,nullptr,data.data(),pitch)<0) {std::cerr<<SDL_GetError();return 1;}
        if(SDL_RenderCopy(r,t,nullptr,nullptr)<0) return 1;
        uint8_t pixel[4];SDL_Rect center{320,180,1,1};
        if(SDL_RenderReadPixels(r,&center,SDL_PIXELFORMAT_RGBA32,pixel,4)<0) {std::cerr<<SDL_GetError();return 1;}
        for(int i=0;i<3;++i)if(pixel[i]<120||pixel[i]>140) {std::cerr<<"Bad pixel "<<static_cast<int>(pixel[i]);return 1;}
        SDL_RenderPresent(r);SDL_DestroyTexture(t);
        std::cout<<"PASS GPU upload/render/readback with padded stride: "<<SDL_GetPixelFormatName(format)<<"\n";
    }
    // Noninteger resizing must interpolate a sharp edge instead of duplicating pixels.
    auto *edge=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STATIC,2,2);
    uint8_t image[]={0,0,0,255,255,255,0,0,0,255,255,255};
    if(!edge||SDL_UpdateTexture(edge,nullptr,image,6)<0)return 1;
    SDL_SetTextureScaleMode(edge,SDL_ScaleModeLinear);
    SDL_RenderCopy(r,edge,nullptr,nullptr);
    uint8_t pixel[4];SDL_Rect middle{320,180,1,1};
    if(SDL_RenderReadPixels(r,&middle,SDL_PIXELFORMAT_RGBA32,pixel,4)<0)return 1;
    if(pixel[0]<110||pixel[0]>145){std::cerr<<"Edge was not smoothly interpolated";return 1;}
    SDL_DestroyTexture(edge);
    std::cout<<"PASS GPU linear scaling: a resized edge interpolates instead of becoming blocky\n";
    SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();
}
