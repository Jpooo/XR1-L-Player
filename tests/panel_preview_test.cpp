#include "../src/menu_ui.hpp"
#include <cassert>
#include <iostream>

int main() {
    SDL_SetMainReady();
    const bool accelerated=std::getenv("XR1_PANEL_GPU")!=nullptr;
    if(!accelerated){
        SDL_setenv("SDL_RENDER_DRIVER","software",1);
#ifndef _WIN32
        SDL_setenv("SDL_VIDEODRIVER","dummy",1);
#endif
    }
    assert(SDL_Init(SDL_INIT_VIDEO)==0&&TTF_Init()==0);
    auto *window=SDL_CreateWindow("Preview fit test",0,0,1100,650,SDL_WINDOW_HIDDEN);
    assert(window);
    if(std::getenv("XR1_PANEL_SCALED"))SDL_SetWindowSize(window,1920,1080);
    auto *renderer=SDL_CreateRenderer(window,-1,accelerated?SDL_RENDERER_ACCELERATED:SDL_RENDERER_SOFTWARE);
    assert(renderer);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    {
        SDL_RenderSetLogicalSize(renderer,1100,650);
        int outputWidth=0,outputHeight=0;
        SDL_GetRendererOutputSize(renderer,&outputWidth,&outputHeight);
        Painter painter(renderer);
        std::vector<Uint32> pixels(size_t(outputWidth)*outputHeight);
        auto sample=[&](float x,float y){
            int outputX=0,outputY=0;
            SDL_RenderLogicalToWindow(renderer,x,y,&outputX,&outputY);
            return pixels[size_t(outputY)*outputWidth+outputX]&0xffffff;
        };
        auto draw=[&](SDL_Texture *texture){
            ui::menuBackground(painter);
            assert(ui::logoPanel(painter,0,true,texture));
            SDL_RenderSetLogicalSize(renderer,0,0);
            assert(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,pixels.data(),outputWidth*4)==0);
            SDL_RenderSetLogicalSize(renderer,1100,650);
        };
        const auto &area=ui::previewArea;
        auto verify=[&](int width,int height,float fittedWidth,float fittedHeight,Uint32 format,int access){
            auto *texture=SDL_CreateTexture(renderer,format,access,width,height);
            assert(texture);
            SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
            const bool rgb=format==SDL_PIXELFORMAT_RGB24;
            const bool planar=format==SDL_PIXELFORMAT_NV12||format==SDL_PIXELFORMAT_IYUV||format==SDL_PIXELFORMAT_YV12;
            const int pitch=planar?width:width*(rgb?3:2);
            std::vector<Uint8> data(size_t(pitch)*(planar?height*3/2:height),128);
            if(rgb)std::fill(data.begin(),data.end(),255);
            else if(planar)std::fill_n(data.begin(),size_t(width)*height,235);
            else for(size_t i=format==SDL_PIXELFORMAT_UYVY?1:0;i<data.size();i+=2)data[i]=235;
            assert(SDL_UpdateTexture(texture,nullptr,data.data(),pitch)==0);
            draw(texture);

            const float centerX=area.x+area.w/2.f,centerY=area.y+area.h/2.f;
            const float left=centerX-fittedWidth/2,right=centerX+fittedWidth/2;
            const float top=centerY-fittedHeight/2,bottom=centerY+fittedHeight/2;
            // The four corners and center must all be filled, including the
            // corners the old chamfered mask would have clipped.
            for(auto point:{SDL_FPoint{left+4,top+4},{right-4,top+4},
                            {left+4,bottom-4},{right-4,bottom-4},{centerX,centerY}}){
                auto value=sample(point.x,point.y);
                for(int shift:{0,8,16})assert((value>>shift&255)>240);
            }
            for(auto point:{SDL_FPoint{left-4,centerY},{right+4,centerY},
                            {centerX,top-4},{centerX,bottom+4}})
                assert(sample(point.x,point.y)==0);

            if(rgb){
                // A thin red border in the source must survive on all four
                // sides of the fitted image. Its white interior stays intact.
                const int edge=std::min(width,height)/32;
                for(int y=0;y<height;++y)for(int x=0;x<width;++x){
                    if(x<edge||x>=width-edge||y<edge||y>=height-edge){
                        size_t index=(size_t(y)*width+x)*3;
                        data[index+1]=data[index+2]=0;
                    }
                }
                assert(SDL_UpdateTexture(texture,nullptr,data.data(),pitch)==0);
                draw(texture);
                for(auto point:{SDL_FPoint{left+4,centerY},{right-4,centerY},
                                {centerX,top+4},{centerX,bottom-4}})
                    assert(sample(point.x,point.y)==0xff0000);
                assert(sample(centerX,centerY)==0xffffff);
            }
            SDL_DestroyTexture(texture);
            std::cout<<"PASS fitted "<<width<<'x'<<height<<" preview / "<<SDL_GetPixelFormatName(format)
                     <<" / "<<(access==SDL_TEXTUREACCESS_STATIC?"static":"streaming")<<'\n';
        };
        for(int access:{SDL_TEXTUREACCESS_STATIC,SDL_TEXTUREACCESS_STREAMING}){
            for(Uint32 format:{SDL_PIXELFORMAT_RGB24,SDL_PIXELFORMAT_YUY2,SDL_PIXELFORMAT_UYVY,
                               SDL_PIXELFORMAT_NV12,SDL_PIXELFORMAT_IYUV,SDL_PIXELFORMAT_YV12})
                verify(1920,1080,area.h*16.f/9,area.h,format,access);
            verify(640,480,area.h*4.f/3,area.h,SDL_PIXELFORMAT_RGB24,access);
            verify(2560,1080,area.w,area.w*1080.f/2560,SDL_PIXELFORMAT_RGB24,access);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}
