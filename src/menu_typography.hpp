#pragma once
#include "core.hpp"
#include <SDL_ttf.h>
#include <unordered_map>
#include <stdexcept>
#include <limits>

namespace ui {
// Rasterize once at drawable density. Animation only modulates cached textures;
// it never rerasterizes fonts or touches the capture/rendered video texture.
class MenuLettering {
public:
    struct Label {
        SDL_Texture *shadow=nullptr,*glow=nullptr,*face=nullptr,*shine=nullptr;
        SDL_Rect ink{};int w=0,h=0,pad=0;float scale=1;bool strong=false;
    };
private:
    SDL_Renderer *renderer;
    std::unordered_map<std::string,Label> cache;
    static void release(Label &label){for(auto *texture:{label.shadow,label.glow,label.face,label.shine})if(texture)SDL_DestroyTexture(texture);}
    SDL_Texture *texture(const std::vector<Uint32> &pixels,int w,int h){
        auto *result=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,w,h);
        if(!result)throw std::runtime_error("Cannot create menu lettering: "+std::string(SDL_GetError()));
        if(SDL_UpdateTexture(result,nullptr,pixels.data(),w*4)<0){SDL_DestroyTexture(result);throw std::runtime_error("Cannot upload menu lettering: "+std::string(SDL_GetError()));}
        SDL_SetTextureBlendMode(result,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(result,SDL_ScaleModeLinear);return result;
    }
public:
    explicit MenuLettering(SDL_Renderer *value):renderer(value){}
    MenuLettering(const MenuLettering&)=delete;MenuLettering &operator=(const MenuLettering&)=delete;
    ~MenuLettering(){clear();}
    void clear(){for(auto &item:cache)release(item.second);cache.clear();}
    size_t size()const{return cache.size();}
    const Label &get(TTF_Font *font,const std::string &text,SDL_Color accent,bool strong,float scale){
        auto key=std::to_string(reinterpret_cast<uintptr_t>(font))+":"+std::to_string(scale)+":"+std::to_string(accent.r)+":"+std::to_string(accent.g)+":"+std::to_string(accent.b)+":"+(strong?"1:":"0:")+text;
        auto found=cache.find(key);if(found!=cache.end())return found->second;
        auto *glyphs=TTF_RenderUTF8_Blended(font,text.c_str(),{255,255,255,255});
        if(!glyphs)throw std::runtime_error("Cannot render menu lettering: "+std::string(TTF_GetError()));
        Label label;label.scale=scale;label.strong=strong;label.pad=int(std::ceil(9*scale));label.w=glyphs->w+2*label.pad;label.h=glyphs->h+2*label.pad;
        int left=glyphs->w,top=glyphs->h,right=0,bottom=0;
        std::vector<float> mask(size_t(label.w)*label.h),horizontal(mask.size()),blur(mask.size()),expanded(mask.size());
        for(int y=0;y<glyphs->h;++y)for(int x=0;x<glyphs->w;++x){
            Uint32 pixel=0;std::memcpy(&pixel,static_cast<Uint8*>(glyphs->pixels)+y*glyphs->pitch+x*4,4);Uint8 r,g,b,a;SDL_GetRGBA(pixel,glyphs->format,&r,&g,&b,&a);
            mask[size_t(y+label.pad)*label.w+x+label.pad]=a/255.f;
            if(a>8){left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);}
        }
        SDL_FreeSurface(glyphs);label.ink={left,top,std::max(0,right-left),std::max(0,bottom-top)};
        const int radius=std::max(1,int(std::ceil(3*scale))),edge=std::max(1,int(std::lround(scale))),shadowOffset=int(std::lround(3*scale));
        std::vector<float> kernel(size_t(radius*2+1));float sum=0,sigma=1.2f*scale;
        for(int i=-radius;i<=radius;++i){float value=std::exp(-i*i/(2*sigma*sigma));kernel[size_t(i+radius)]=value;sum+=value;}
        for(auto &value:kernel)value/=sum;
        const int w=label.w,h=label.h;
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){
            for(int k=-radius;k<=radius;++k)if(x+k>=0&&x+k<w)horizontal[size_t(y)*w+x]+=mask[size_t(y)*w+x+k]*kernel[size_t(k+radius)];
            for(int k=-edge;k<=edge;++k)if(x+k>=0&&x+k<w)expanded[size_t(y)*w+x]=std::max(expanded[size_t(y)*w+x],mask[size_t(y)*w+x+k]);
        }
        auto wide=expanded;
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){
            for(int k=-radius;k<=radius;++k)if(y+k>=0&&y+k<h)blur[size_t(y)*w+x]+=horizontal[size_t(y+k)*w+x]*kernel[size_t(k+radius)];
            for(int k=-edge;k<=edge;++k)if(y+k>=0&&y+k<h)expanded[size_t(y)*w+x]=std::max(expanded[size_t(y)*w+x],wide[size_t(y+k)*w+x]);
        }
        auto *format=SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
        if(!format)throw std::runtime_error("Cannot allocate menu lettering format");
        std::vector<Uint32> pixels(mask.size());
        auto alpha=[](float value){return Uint8(std::lround(std::clamp(value,0.f,1.f)*255));};
        try{
            for(int y=0;y<h;++y)for(int x=0;x<w;++x){float a=y>=shadowOffset?blur[size_t(y-shadowOffset)*w+x]:0;pixels[size_t(y)*w+x]=SDL_MapRGBA(format,0,1,3,alpha(a*235/255));}
            label.shadow=texture(pixels,w,h);
            for(size_t i=0;i<pixels.size();++i)pixels[i]=SDL_MapRGBA(format,accent.r,accent.g,accent.b,alpha(blur[i]*(strong?175.f:65.f)/255));
            label.glow=texture(pixels,w,h);
            for(int y=0;y<h;++y)for(int x=0;x<w;++x){
                size_t i=size_t(y)*w+x;float face=mask[i],rim=y>=edge?mask[size_t(y-edge)*w+x]*(strong?180.f:100.f)/255:0,keyline=expanded[i]*235/255;
                float t=std::clamp(float(y-label.pad-top)/std::max(1,label.ink.h-1),0.f,1.f),white=strong?.52f-.42f*t:1;
                float a=face+rim*(1-face)+keyline*(1-face)*(1-rim);
                auto channel=[&](Uint8 color,int dark){float value=strong?color+(255-color)*white:255-19*t;return a?Uint8(std::lround(std::clamp((value*face+color*rim*(1-face)+dark*keyline*(1-face)*(1-rim))/a,0.f,255.f))):Uint8(0);};
                pixels[i]=SDL_MapRGBA(format,channel(accent.r,1),channel(accent.g,7),channel(accent.b,9),alpha(a));
            }
            label.face=texture(pixels,w,h);
            for(size_t i=0;i<pixels.size();++i)pixels[i]=SDL_MapRGBA(format,255,255,255,alpha(mask[i]));
            label.shine=texture(pixels,w,h);
        }catch(...){SDL_FreeFormat(format);release(label);throw;}
        SDL_FreeFormat(format);
        if(cache.size()>=32)clear();
        return cache.emplace(std::move(key),label).first->second;
    }
    void draw(const Label &label,float x,float y,double seconds,bool animate){
        float sx=1,sy=1;SDL_RenderGetScale(renderer,&sx,&sy);
        auto logical=[](float pixel,float scale){return std::nextafter(pixel/scale,std::numeric_limits<float>::infinity());};
        float px=std::round(x*sx)-label.pad,py=std::round(y*sy)-label.pad;
        SDL_FRect dst{logical(px,sx),logical(py,sy),logical(float(label.w),sx),logical(float(label.h),sy)};
        SDL_RenderCopyF(renderer,label.shadow,nullptr,&dst);
        float phase=animate?float(std::fmod(seconds,3.0)/3.0):.28f;
        float breath=label.strong?.8f+.2f*std::cos(phase*6.2831853f):1;
        SDL_SetTextureAlphaMod(label.glow,Uint8(std::lround(breath*255)));SDL_RenderCopyF(renderer,label.glow,nullptr,&dst);
        SDL_RenderCopyF(renderer,label.face,nullptr,&dst);
        if(label.strong){
            float center=label.pad+label.ink.x-25*label.scale+phase*(label.ink.w+65*label.scale);
            int step=std::max(1,int(std::lround(3*label.scale)));
            for(int i=-5;i<=5;++i){
                int start=int(std::lround(center))+i*step,left=std::max(0,start),right=std::min(label.w,start+step);if(right<=left)continue;
                auto opacity=Uint8(std::lround(70*std::exp(-std::pow((i+.5f)*step/(10*label.scale),2.f))));SDL_SetTextureAlphaMod(label.shine,opacity);
                SDL_Rect src{left,0,right-left,label.h};SDL_FRect band{logical(px+left,sx),logical(py,sy),logical(float(src.w),sx),logical(float(src.h),sy)};
                SDL_RenderCopyF(renderer,label.shine,&src,&band);
            }
        }
    }
};
}
