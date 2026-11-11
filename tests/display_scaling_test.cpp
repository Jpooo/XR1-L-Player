#include "../src/ui.hpp"
#include <cassert>
#include <iostream>

int main(){
    SDL_SetMainReady();assert(SDL_Init(SDL_INIT_VIDEO)==0&&TTF_Init()==0);
    auto *window=SDL_CreateWindow("Menu display scaling",0,0,1100,650,SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE);assert(window);
    auto *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);assert(renderer);
    {
        Painter p(renderer);
        // Include 125/150/200% menu sizes, 720p, laptop, 1080p, 1440p,
        // ultrawide and 4K drawables, then return to the small window.
        const int dimensions[][2]={{900,560},{1100,650},{1280,720},{1366,768},{1375,813},{1650,975},{1920,1080},{2200,1300},{2560,1440},{3440,1440},{3840,2160},{1100,650}};
        for(auto &size:dimensions){
            SDL_SetWindowSize(window,size[0],size[1]);SDL_PumpEvents();
            SDL_RenderSetLogicalSize(renderer,1100,650);p.prepare();
            int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);assert(w==size[0]&&h==size[1]);
            std::vector<Uint32> actual(size_t(w)*h),reference(actual.size());
            for(auto *font:{p.small,p.normal,p.heading,p.title}){
                SDL_RenderSetLogicalSize(renderer,1100,650);
                SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
                p.text("Settings 1080p60",103,62,{255,255,255,255},font);
                SDL_RenderSetLogicalSize(renderer,0,0);
                assert(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,actual.data(),w*4)==0);
                SDL_RenderSetLogicalSize(renderer,1100,650);
                float sx=0,sy=0;SDL_RenderGetScale(renderer,&sx,&sy);int ox=0,oy=0;SDL_RenderLogicalToWindow(renderer,0,0,&ox,&oy);
                auto *glyphs=TTF_RenderUTF8_Blended(font,"Settings 1080p60",{255,255,255,255});assert(glyphs);
                auto *texture=SDL_CreateTextureFromSurface(renderer,glyphs);assert(texture);
                SDL_Rect target{ox+int(std::round(103*sx)),oy+int(std::round(62*sy)),glyphs->w,glyphs->h};SDL_FreeSurface(glyphs);
                SDL_RenderSetLogicalSize(renderer,0,0);SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer,texture,nullptr,&target);
                assert(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,reference.data(),w*4)==0);
                int maximum=0;for(size_t i=0;i<actual.size();++i)maximum=std::max(maximum,std::abs(int(actual[i]&255)-int(reference[i]&255)));
                if(maximum>2)std::cerr<<w<<'x'<<h<<" text reference difference "<<maximum<<std::endl;
                assert(maximum<=2);SDL_DestroyTexture(texture);
            }
            SDL_RenderSetLogicalSize(renderer,1100,650);
            // A higher-density mask must retain a narrow antialiased edge,
            // rather than stretching a base-resolution edge across 3 pixels.
            p.round({100,100,80,40},10,{255,255,255,255});assert(!p.shapes.empty());
            auto *mask=p.shapes.begin()->second;int mw=0,mh=0;SDL_QueryTexture(mask,nullptr,nullptr,&mw,&mh);
            assert(mw>=int(80*p.fontScale)-1&&mh>=int(40*p.fontScale)-1);
            int x=0,y=0;SDL_RenderLogicalToWindow(renderer,220,360,&x,&y);float lx=0,ly=0;SDL_RenderWindowToLogical(renderer,x,y,&lx,&ly);
            assert(std::abs(lx-220)<2&&std::abs(ly-360)<2);
            std::cout<<"PASS physical-resolution text, menu masks and input transform at "<<w<<'x'<<h<<'\n';
        }
    }
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();
}
