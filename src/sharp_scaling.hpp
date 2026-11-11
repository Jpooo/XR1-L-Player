#pragma once
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <climits>

// Fullscreen fractional-upscale path only. It does not queue capture frames or
// alter the native 1080p/integer-scale paths. Caller resets before renderer loss.
class SharpFullscreenScaling {
    SDL_Renderer *owner=nullptr;
    SDL_Texture *target=nullptr,*rgbState=nullptr;
    int targetWidth=0,targetHeight=0;
    bool failed=false,requireAcceleration;
    void release(){if(target)SDL_DestroyTexture(target);if(rgbState)SDL_DestroyTexture(rgbState);target=rgbState=nullptr;targetWidth=targetHeight=0;}
    struct State {
        SDL_Renderer *r;SDL_Texture *target;
        int logicalW=0,logicalH=0;float sx=1,sy=1;
        SDL_Rect viewport{},clip{};SDL_bool clipped,integer;
        Uint8 red=0,green=0,blue=0,alpha=255;SDL_BlendMode blend=SDL_BLENDMODE_NONE;
        bool restored=false;
        explicit State(SDL_Renderer *renderer):r(renderer),target(SDL_GetRenderTarget(r)),clipped(SDL_RenderIsClipEnabled(r)),integer(SDL_RenderGetIntegerScale(r)){
            SDL_RenderGetLogicalSize(r,&logicalW,&logicalH);SDL_RenderGetScale(r,&sx,&sy);SDL_RenderGetViewport(r,&viewport);SDL_RenderGetClipRect(r,&clip);
            SDL_GetRenderDrawColor(r,&red,&green,&blue,&alpha);SDL_GetRenderDrawBlendMode(r,&blend);
        }
        bool restore(){
            if(restored)return true;
            bool ok=SDL_SetRenderTarget(r,target)==0;
            ok=(SDL_RenderSetIntegerScale(r,integer)==0)&&ok;
            ok=(SDL_RenderSetLogicalSize(r,logicalW,logicalH)==0)&&ok;
            ok=(SDL_RenderSetScale(r,sx,sy)==0)&&ok;
            ok=(SDL_RenderSetViewport(r,&viewport)==0)&&ok;
            ok=(SDL_RenderSetClipRect(r,clipped?&clip:nullptr)==0)&&ok;
            ok=(SDL_SetRenderDrawColor(r,red,green,blue,alpha)==0)&&ok;
            ok=(SDL_SetRenderDrawBlendMode(r,blend)==0)&&ok;
            restored=true;return ok;
        }
        ~State(){restore();}
    };
    static int linear(SDL_Renderer *r,SDL_Texture *source,const SDL_Rect &destination){
        SDL_ScaleMode old=SDL_ScaleModeLinear;
        if(SDL_GetTextureScaleMode(source,&old)<0)return SDL_RenderCopy(r,source,nullptr,&destination);
        if(SDL_SetTextureScaleMode(source,SDL_ScaleModeLinear)<0)return -1;
        int result=SDL_RenderCopy(r,source,nullptr,&destination);SDL_SetTextureScaleMode(source,old);return result;
    }
public:
    explicit SharpFullscreenScaling(bool accelerationRequired=true):requireAcceleration(accelerationRequired){}
    SharpFullscreenScaling(const SharpFullscreenScaling&)=delete;
    SharpFullscreenScaling &operator=(const SharpFullscreenScaling&)=delete;
    ~SharpFullscreenScaling(){reset();}
    void reset(){release();owner=nullptr;failed=false;}
    int render(SDL_Renderer *r,SDL_Texture *source,const SDL_Rect &destination){
        if(!r||!source)return SDL_SetError("Missing renderer or video texture");
        int sourceW=0,sourceH=0;Uint32 format=0;
        if(SDL_QueryTexture(source,&format,nullptr,&sourceW,&sourceH)<0)return -1;
        if(sourceW!=1920||sourceH!=1080||destination.w<=sourceW||destination.h<=sourceH||
           (destination.w%sourceW==0&&destination.h%sourceH==0&&destination.w/sourceW==destination.h/sourceH))return linear(r,source,destination);
        if(owner!=r){reset();owner=r;}
        if(failed)return linear(r,source,destination);
        SDL_RendererInfo info{};
        if(SDL_GetRendererInfo(r,&info)<0||!(info.flags&SDL_RENDERER_TARGETTEXTURE)||(requireAcceleration&&!(info.flags&SDL_RENDERER_ACCELERATED))){failed=true;return linear(r,source,destination);}
        const int factor=int(std::ceil(std::max(double(destination.w)/sourceW,double(destination.h)/sourceH)));
        const Sint64 wide=Sint64(sourceW)*factor,tall=Sint64(sourceH)*factor;
        if(wide>INT_MAX||tall>INT_MAX){failed=true;return linear(r,source,destination);}
        const int width=int(wide),height=int(tall);
        if((info.max_texture_width>0&&width>info.max_texture_width)||(info.max_texture_height>0&&height>info.max_texture_height)){failed=true;return linear(r,source,destination);}
        State state(r);SDL_ScaleMode sourceFilter=SDL_ScaleModeLinear;
        if(SDL_GetTextureScaleMode(source,&sourceFilter)<0)return linear(r,source,destination);
        auto fallback=[&](){
            SDL_SetTextureScaleMode(source,sourceFilter);
            bool restored=state.restore();failed=true;release();
            return restored?linear(r,source,destination):-1;
        };
        if(!target||targetWidth!=width||targetHeight!=height){
            release();
            target=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,width,height);
            rgbState=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,1,1);
            if(!target||!rgbState)return fallback();
            targetWidth=width;targetHeight=height;Uint32 black=0xff000000;
            if(SDL_UpdateTexture(rgbState,nullptr,&black,4)<0||SDL_SetTextureBlendMode(rgbState,SDL_BLENDMODE_NONE)<0||SDL_SetTextureBlendMode(target,SDL_BLENDMODE_NONE)<0||SDL_SetTextureScaleMode(target,SDL_ScaleModeLinear)<0)return fallback();
        }
        if(SDL_SetRenderTarget(r,target)<0||SDL_RenderSetIntegerScale(r,SDL_FALSE)<0||SDL_RenderSetLogicalSize(r,0,0)<0||
           SDL_RenderSetScale(r,1,1)<0||SDL_RenderSetViewport(r,nullptr)<0||SDL_RenderSetClipRect(r,nullptr)<0||
           SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE)<0||SDL_SetRenderDrawColor(r,0,0,0,255)<0||SDL_RenderClear(r)<0||
           SDL_SetTextureScaleMode(source,SDL_ScaleModeNearest)<0)return fallback();
        if(format==SDL_PIXELFORMAT_NV12){
            // SDL's GL NV12 path can retain stale multitexture/program state on
            // an FBO switch. A real in-bounds RGB draw invalidates that state;
            // flush alone was insufficient. The video immediately covers it.
            SDL_Rect pixel{0,0,1,1};
            if(SDL_RenderCopy(r,rgbState,nullptr,&pixel)<0||SDL_RenderFlush(r)<0)return fallback();
        }
        if(SDL_RenderCopy(r,source,nullptr,nullptr)<0||SDL_RenderFlush(r)<0)return fallback();
        SDL_SetTextureScaleMode(source,sourceFilter);
        if(!state.restore()){failed=true;release();return -1;}
        if(SDL_RenderCopy(r,target,nullptr,&destination)<0){failed=true;release();return linear(r,source,destination);}
        return 0;
    }
};
