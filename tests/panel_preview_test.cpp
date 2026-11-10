#include "../src/menu_ui.hpp"
#include <cassert>
#include <iostream>
int main(){
    SDL_SetMainReady();
    if(!std::getenv("XR1_PANEL_GPU")){
        SDL_setenv("SDL_RENDER_DRIVER","software",1);
#ifndef _WIN32
        SDL_setenv("SDL_VIDEODRIVER","dummy",1);
#endif
    }
    assert(SDL_Init(SDL_INIT_VIDEO)==0&&TTF_Init()==0);
    auto *window=SDL_CreateWindow("Panel fill test",0,0,1100,650,SDL_WINDOW_HIDDEN);assert(window);
    if(std::getenv("XR1_PANEL_SCALED"))SDL_SetWindowSize(window,1920,1080);
    auto *r=SDL_CreateRenderer(window,-1,std::getenv("XR1_PANEL_GPU")?SDL_RENDERER_ACCELERATED:SDL_RENDERER_SOFTWARE);assert(r);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    {
        SDL_RenderSetLogicalSize(r,1100,650);int outputW=0,outputH=0;SDL_GetRendererOutputSize(r,&outputW,&outputH);
        Painter painter(r);std::vector<Uint32> baseline(size_t(outputW)*outputH),result(baseline.size());
        auto pixel=[&](int x,int y){int px=0,py=0;SDL_RenderLogicalToWindow(r,x,y,&px,&py);return size_t(py)*outputW+px;};
        auto inside=[](float x,float y){bool found=false;const auto &v=ui::mainPanelContour;for(size_t i=0,j=v.size()-1;i<v.size();j=i++)if((v[i].y>y)!=(v[j].y>y)&&x<(v[j].x-v[i].x)*(y-v[i].y)/(v[j].y-v[i].y)+v[i].x)found=!found;return found;};
        auto edgeDistance=[](float x,float y){float distance=1e9f;const auto &v=ui::mainPanelContour;for(size_t i=0;i<v.size();++i){auto a=v[i],b=v[(i+1)%v.size()];float dx=b.x-a.x,dy=b.y-a.y,t=std::clamp(((x-a.x)*dx+(y-a.y)*dy)/(dx*dx+dy*dy),0.0f,1.0f);distance=std::min(distance,std::hypot(x-a.x-t*dx,y-a.y-t*dy));}return distance;};
        auto draw=[&](SDL_Texture *video,std::vector<Uint32> &pixels){SDL_SetRenderDrawColor(r,4,7,10,255);SDL_RenderClear(r);painter.motion=false;ui::menuBackground(painter);bool shown=ui::logoPanel(painter,0,true,video);assert(shown==(video!=nullptr));SDL_RenderSetLogicalSize(r,0,0);assert(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,pixels.data(),outputW*4)==0);SDL_RenderSetLogicalSize(r,1100,650);};
        draw(nullptr,baseline);
        for(int access:{SDL_TEXTUREACCESS_STATIC,SDL_TEXTUREACCESS_STREAMING})for(Uint32 format:{SDL_PIXELFORMAT_RGB24,SDL_PIXELFORMAT_YUY2,SDL_PIXELFORMAT_UYVY,SDL_PIXELFORMAT_NV12,SDL_PIXELFORMAT_IYUV,SDL_PIXELFORMAT_YV12}){
            auto *texture=SDL_CreateTexture(r,format,access,1920,1080);assert(texture);SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
            bool planar=format==SDL_PIXELFORMAT_NV12||format==SDL_PIXELFORMAT_IYUV||format==SDL_PIXELFORMAT_YV12;
            int pitch=planar?1920:format==SDL_PIXELFORMAT_RGB24?5760:3840;
            std::vector<Uint8> data(size_t(pitch)*(planar?1620:1080));
            for(bool white:{false,true}){
                std::fill(data.begin(),data.end(),128);
                if(format==SDL_PIXELFORMAT_RGB24)std::fill(data.begin(),data.end(),white?255:0);
                else if(planar)std::fill_n(data.begin(),1920*1080,white?235:16);
                else for(size_t i=format==SDL_PIXELFORMAT_UYVY?1:0;i<data.size();i+=2)data[i]=white?235:16;
                assert(SDL_UpdateTexture(texture,nullptr,data.data(),pitch)==0);draw(texture,result);
                // These points used to sit in the empty inset gutters. They must
                // now contain source video right up to the panel's frame.
                for(auto point:{SDL_Point{465,300},SDL_Point{1040,340},SDL_Point{700,185},SDL_Point{750,505},SDL_Point{755,346}}){
                    auto value=result[pixel(point.x,point.y)];for(int shift:{0,8,16}){int c=(value>>shift)&255;assert(white?c>240:c<15);}
                }
                // The notch and chamfer cutouts must retain the menu background,
                // not expose the rectangular video bounds beyond the panel.
                for(auto point:{SDL_Point{456,180},SDL_Point{642,175},SDL_Point{1055,506},SDL_Point{456,512}}){
                    size_t i=pixel(point.x,point.y);for(int shift:{0,8,16})assert(std::abs(int((result[i]>>shift)&255)-int((baseline[i]>>shift)&255))<=2);
                }
            }
            for(int y=180;y<518;y+=13)for(int x=452;x<1058;x+=13){
                if(edgeDistance(float(x),float(y))<6)continue;
                size_t i=pixel(x,y);
                for(int shift:{0,8,16}){
                    int actual=(result[i]>>shift)&255;
                    if(inside(float(x),float(y)))assert(actual>240);
                    else assert(std::abs(actual-int((baseline[i]>>shift)&255))<=2);
                }
            }
            SDL_DestroyTexture(texture);std::cout<<"PASS panel fill / masked corners / "<<SDL_GetPixelFormatName(format)<<" / "<<(access==SDL_TEXTUREACCESS_STATIC?"static":"streaming")<<"\n";
        }

    }
    SDL_DestroyRenderer(r);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();
}
