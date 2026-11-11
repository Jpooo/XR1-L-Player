#include "../src/menu_ui.hpp"
#include <cassert>
#include <iostream>

namespace {
struct Bounds {
    int left=0,top=0,right=0,bottom=0;
    bool valid=false;
    void include(int x,int y){
        if(!valid){left=x;right=x+1;top=y;bottom=y+1;valid=true;}
        else {left=std::min(left,x);right=std::max(right,x+1);top=std::min(top,y);bottom=std::max(bottom,y+1);}
    }
};
Bounds glyphBounds(TTF_Font *font,const std::string &text){
    auto *surface=TTF_RenderUTF8_Blended(font,text.c_str(),{255,255,255,255});assert(surface);
    Bounds result;
    for(int y=0;y<surface->h;++y)for(int x=0;x<surface->w;++x){
        Uint32 pixel=0;std::memcpy(&pixel,static_cast<Uint8*>(surface->pixels)+y*surface->pitch+x*4,4);
        Uint8 r,g,b,a;SDL_GetRGBA(pixel,surface->format,&r,&g,&b,&a);if(a>8)result.include(x,y);
    }
    SDL_FreeSurface(surface);assert(result.valid);return result;
}
Bounds brightBounds(const std::vector<Uint32> &pixels,int width){
    Bounds result;
    for(size_t i=0;i<pixels.size();++i){
        auto value=pixels[i];int r=(value>>16)&255,g=(value>>8)&255,b=value&255;
        // The sharp face is bright; exclude its diffuse glow and dark shadow.
        if(std::max({r,g,b})>=190)result.include(int(i%width),int(i/width));
    }
    assert(result.valid);return result;
}
std::vector<Uint32> readPixels(SDL_Renderer *renderer,int width,int height){
    SDL_RenderSetLogicalSize(renderer,0,0);std::vector<Uint32> result(size_t(width)*height);
    assert(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,result.data(),width*4)==0);
    SDL_RenderSetLogicalSize(renderer,1100,650);return result;
}
int themePixels(const std::vector<Uint32> &pixels,ui::ConsoleColor theme){
    int count=0;
    for(auto value:pixels){int r=(value>>16)&255,g=(value>>8)&255,b=value&255;
        if(theme==ui::ConsoleColor::Xbox&&g>180&&g>r+20&&g>b+40)++count;
        if(theme==ui::ConsoleColor::PlayStation&&b>180&&b>r+30&&b>g+20)++count;
        if(theme==ui::ConsoleColor::Nintendo&&r>180&&r>g+30&&r>b+30)++count;
    }
    return count;
}
int neutralPixels(const std::vector<Uint32> &pixels){
    return int(std::count_if(pixels.begin(),pixels.end(),[](Uint32 v){return ((v>>16)&255)>220&&((v>>8)&255)>220&&(v&255)>220;}));
}
}

int main(){
    SDL_SetMainReady();
#ifndef _WIN32
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);
#endif
    assert(SDL_Init(SDL_INIT_VIDEO)==0&&TTF_Init()==0);
    auto *window=SDL_CreateWindow("Menu typography regression",0,0,1100,650,SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE);assert(window);
    auto *renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);assert(renderer);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    {
        SDL_RenderSetLogicalSize(renderer,1100,650);Painter p(renderer);
        const int sizes[][2]={{1100,650},{1375,813},{3840,2160},{1100,650}};
        for(auto &size:sizes){
            SDL_SetWindowSize(window,size[0],size[1]);SDL_PumpEvents();SDL_RenderSetLogicalSize(renderer,1100,650);p.prepare();
            assert(p.menuLetters.size()==0);int width=0,height=0;SDL_GetRendererOutputSize(renderer,&width,&height);assert(width==size[0]&&height==size[1]);
            float sx=0,sy=0;SDL_RenderGetScale(renderer,&sx,&sy);int ox=0,oy=0;SDL_RenderLogicalToWindow(renderer,0,0,&ox,&oy);
            auto render=[&](const auto &label,float x,float y,double seconds,bool animate){
                SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);p.menuLetters.draw(label,x,y,seconds,animate);return readPixels(renderer,width,height);
            };
            for(int index=0;index<3;++index){
                const std::string text=index==0?"PLAY":index==1?"SETTINGS":"PREFERENCES";auto *font=index==0?p.title:p.heading;
                auto independent=glyphBounds(font,text);auto &label=p.menuLetters.get(font,text,p.theme.accent(),index==0,p.fontScale);
                assert(label.ink.x==independent.left&&label.ink.y==independent.top);
                assert(label.ink.w==independent.right-independent.left&&label.ink.h==independent.bottom-independent.top);
                auto box=ui::navigation[size_t(index)];float cx=box.x+box.w/2.f,cy=box.y+box.h/2.f;
                float x=cx-(independent.left+independent.right)/(2*p.fontScale),y=cy-(independent.top+independent.bottom)/(2*p.fontScale);
                // Check the visible word itself has clearance from both side icons.
                assert(cx-label.ink.w/(2*p.fontScale)>box.x+64);
                assert(cx+label.ink.w/(2*p.fontScale)<box.x+box.w-44);
                auto first=render(label,x,y,0,false),still=render(label,x,y,1.5,false);assert(first==still);
                auto bounds=brightBounds(first,width);
                assert(std::abs((bounds.left+bounds.right)/2.f-(ox+cx*sx))<=2);
                assert(std::abs((bounds.top+bounds.bottom)/2.f-(oy+cy*sy))<=2);
                if(index==0){
                    auto warm=p.menuLetters.size();auto moving=render(label,x,y,0,true),later=render(label,x,y,1.5,true);assert(moving!=later);
                    auto laterBounds=brightBounds(later,width);
                    assert(std::abs((bounds.left+bounds.right)-(laterBounds.left+laterBounds.right))<=2);
                    assert(std::abs((bounds.top+bounds.bottom)-(laterBounds.top+laterBounds.bottom))<=2);
                    for(int frame=0;frame<12;++frame)render(label,x,y,frame*.125,true);
                    assert(p.menuLetters.size()==warm);assert(&p.menuLetters.get(font,text,p.theme.accent(),true,p.fontScale)==&label);
                    assert(themePixels(first,p.theme.choice)>10);
                }else assert(neutralPixels(first)>10);
            }
            p.clearLabels();assert(p.menuLetters.size()==0);
            for(auto theme:{ui::ConsoleColor::Xbox,ui::ConsoleColor::PlayStation,ui::ConsoleColor::Nintendo}){
                p.theme.choice=theme;auto &active=p.menuLetters.get(p.title,"PLAY",p.theme.accent(),true,p.fontScale);
                auto activePixels=render(active,150,200,0,false);assert(themePixels(activePixels,theme)>10);
                auto &inactive=p.menuLetters.get(p.title,"PLAY",p.theme.accent(),false,p.fontScale);
                auto inactivePixels=render(inactive,150,200,0,false);assert(neutralPixels(inactivePixels)>neutralPixels(activePixels));
                p.clearLabels();assert(p.menuLetters.size()==0);
            }
            p.theme.choice=ui::ConsoleColor::Xbox;
            p.menuLetters.get(p.title,"PLAY",p.theme.accent(),true,p.fontScale);assert(p.menuLetters.size()>0);p.resetTextures();assert(p.menuLetters.size()==0);
            p.menuLetters.get(p.title,"PLAY",p.theme.accent(),true,p.fontScale);assert(p.menuLetters.size()==1);
            std::cout<<"PASS styled menu text, ink centering, all themes, Motion Off, cached animation and reset at "<<width<<'x'<<height<<'\n';
        }
    }
    SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();
}
