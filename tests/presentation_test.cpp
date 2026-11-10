#include "../src/video_common.hpp"
#ifndef BENCH_UI_HEADER
#define BENCH_UI_HEADER "../src/ui.hpp"
#endif
#include BENCH_UI_HEADER
#include <cassert>
int main(){
    assert(SDL_Init(SDL_INIT_VIDEO)==0&&TTF_Init()==0);
    auto *window=SDL_CreateWindow("Presentation fidelity",0,0,1920,1080,SDL_WINDOW_HIDDEN);assert(window);
    auto *r=SDL_CreateRenderer(window,-1,std::getenv("XR1_PRESENTATION_GPU")?SDL_RENDERER_ACCELERATED:SDL_RENDERER_SOFTWARE);assert(r);
    {
        // A 1080p capture must survive a 1080p drawable without losing a pixel.
        std::vector<Uint32> source(1920*1080),result(source.size());
        for(int y=0;y<1080;++y)for(int x=0;x<1920;++x)source[y*1920+x]=((x^y)&1)?0xffffffff:0xff000000;
        auto *texture=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,1920,1080);assert(texture);
        SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);assert(SDL_UpdateTexture(texture,nullptr,source.data(),1920*4)==0);
        assert(SDL_RenderCopy(r,texture,nullptr,nullptr)==0);
        assert(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,result.data(),1920*4)==0);assert(source==result);SDL_DestroyTexture(texture);
        std::cout<<"PASS native 1920x1080 single-pixel video detail preserved exactly\n";
        // Compare fullscreen menu text with the original physical-resolution
        // font raster, rather than blessing a blurred screenshot as the reference.
        SDL_RenderSetLogicalSize(r,1100,650);Painter painter(r);
        SDL_SetRenderDrawColor(r,0,0,0,255);SDL_RenderClear(r);painter.text("Home 1080p60",103,62,{255,255,255,255});
        SDL_RenderSetLogicalSize(r,0,0);
        assert(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,result.data(),1920*4)==0);
        SDL_RenderSetLogicalSize(r,1100,650);
        float sx=0,sy=0;SDL_RenderGetScale(r,&sx,&sy);int ox=0,oy=0;SDL_RenderLogicalToWindow(r,0,0,&ox,&oy);
        auto *glyphs=TTF_RenderUTF8_Blended(painter.normal,"Home 1080p60",{255,255,255,255});assert(glyphs);
        texture=SDL_CreateTextureFromSurface(r,glyphs);assert(texture);
        SDL_Rect target{ox+int(std::round(103*sx)),oy+int(std::round(62*sy)),glyphs->w,glyphs->h};SDL_FreeSurface(glyphs);
        SDL_RenderSetLogicalSize(r,0,0);SDL_SetRenderDrawColor(r,0,0,0,255);SDL_RenderClear(r);SDL_RenderCopy(r,texture,nullptr,&target);
        assert(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,source.data(),1920*4)==0);
        double difference=0;int maximum=0;
        for(size_t i=0;i<source.size();++i){int delta=std::abs(int(source[i]&255)-int(result[i]&255));difference+=delta;maximum=std::max(maximum,delta);}
        std::cout<<"Text physical-pixel reference error mean/max: "<<difference/(target.w*target.h)<<" / "<<maximum<<std::endl;

#ifndef ALLOW_TEXT_BLUR
        assert(maximum<=2);
#endif
        SDL_DestroyTexture(texture);
        painter.round({20,20,80,40},8,{100,100,100,255});
        painter.icon(ui::Icon::Controller,40,40,40,{255,255,255,255});
        painter.glow({20,20,80,80},{100,100,100,80});
        painter.gradient({0,0,100,100},{0,0,0,255},{100,100,100,255});
        assert(!painter.labels.empty()&&!painter.shapes.empty()&&!painter.icons.empty()&&painter.glowTexture&&painter.backdrop);
        painter.resetTextures();assert(painter.labels.empty()&&painter.shapes.empty()&&painter.icons.empty()&&!painter.glowTexture&&!painter.backdrop);
        painter.text("Recovered",20,20,{255,255,255,255});assert(!painter.labels.empty());
        std::cout<<"PASS renderer-reset UI caches cleared and text rebuilt\n";
    }
    SDL_DestroyRenderer(r);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();
}
