#include <SDL.h>
static bool failNextCopy=false;
static int checkedCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_Rect *dst){
 if(failNextCopy){failNextCopy=false;return SDL_SetError("Injected conversion failure");}
 return SDL_RenderCopy(r,t,src,dst);
}
#define SDL_RenderCopy checkedCopy
#include "../src/fsr_upscaler.hpp"
#undef SDL_RenderCopy
#include <cassert>
#include <iostream>
#include <filesystem>
int main(){
 const char *overrideAssets=std::getenv("XR1_ASSET_DIR");
 std::string assets=overrideAssets?overrideAssets:(std::filesystem::path(__FILE__).parent_path().parent_path()/"assets").string();
 SDL_SetMainReady();assert(SDL_Init(SDL_INIT_VIDEO)==0);
 bool software=std::getenv("XR1_SOFTWARE_TEST");
 if(!software){SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);}
 auto *w=SDL_CreateWindow("FSR validation",0,0,2560,1440,SDL_WINDOW_HIDDEN);assert(w);
 auto *r=SDL_CreateRenderer(w,-1,software?SDL_RENDERER_SOFTWARE:SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE);assert(r);
 SDL_RendererInfo info{};SDL_GetRendererInfo(r,&info);std::cout<<"Renderer "<<info.name<<std::endl;
 auto *input=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,1920,1080);assert(input);
 std::vector<Uint32> pixels(1920*1080);for(int y=0;y<1080;++y)for(int x=0;x<1920;++x){bool bright=(x>y*16/9);pixels[y*1920+x]=0xff000000u|(x<960?0xa00000:0x200000)|(y<540?0xa000:0x2000)|(bright?0xe0:0x20);}
 SDL_UpdateTexture(input,nullptr,pixels.data(),1920*4);SDL_SetTextureScaleMode(input,SDL_ScaleModeLinear);
 {
  FsrUpscaler fsr;SDL_Rect dst{0,0,2560,1440};
  assert(fsr.render(r,input,dst,assets,true)==0);std::cout<<fsr.status()<<std::endl;
  if(!software)assert(fsr.status().find("active")!=std::string::npos);
  SDL_RenderFlush(r);std::vector<Uint32> result(2560*1440);assert(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,result.data(),2560*4)==0);
  for(auto point:{SDL_Point{200,300},SDL_Point{2240,180},SDL_Point{320,1260},SDL_Point{2240,1000}}){auto got=result[point.y*2560+point.x],expected=pixels[(point.y*3/4)*1920+point.x*3/4];for(int shift:{0,8,16})assert(std::abs(int((got>>shift)&255)-int((expected>>shift)&255))<4);}
  if(!software){
   uint64_t previousDiff=0;
   std::vector<Uint32> sharpened(result.size());
   for(unsigned strength=1;strength<=3;++strength){
    assert(fsr.render(r,input,dst,assets,false,strength)==0);assert(fsr.status().find(fsrSharpLabel(strength))!=std::string::npos);
    SDL_RenderFlush(r);assert(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,sharpened.data(),2560*4)==0);
    uint64_t difference=0;for(size_t i=0;i<result.size();++i)for(int channel:{0,8,16})difference+=std::abs(int((result[i]>>channel)&255)-int((sharpened[i]>>channel)&255));
    assert(difference>previousDiff);previousDiff=difference;
    for(auto point:{SDL_Point{200,300},SDL_Point{2240,180},SDL_Point{320,1260},SDL_Point{2240,1000}}){auto a=sharpened[point.y*2560+point.x],b=result[point.y*2560+point.x];for(int channel:{0,8,16})assert(std::abs(int((a>>channel)&255)-int((b>>channel)&255))<4);}
   }
   assert(fsr.render(r,input,dst,assets,false,0)==0);SDL_RenderFlush(r);assert(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,sharpened.data(),2560*4)==0);assert(sharpened==result);
   std::cout<<"PASS RCAS Low/Medium/High strength differences, flat-color preservation and exact Off restoration\n";
  }
  assert(fsr.render(r,input,dst,assets,false)==0);
  SDL_SetRenderDrawColor(r,7,13,21,255);SDL_Rect patch{0,0,30,30};SDL_RenderFillRect(r,&patch);SDL_RenderFlush(r);Uint32 pixel=0;SDL_Rect one{10,10,1,1};SDL_RenderReadPixels(r,&one,SDL_PIXELFORMAT_ARGB8888,&pixel,4);assert((pixel&0xffffff)==0x070d15);
  if(!software){
   std::vector<Uint32> replacement(pixels.size(),0xff406080u);SDL_UpdateTexture(input,nullptr,replacement.data(),1920*4);
   failNextCopy=true;assert(fsr.render(r,input,dst,assets,true)==0);
   assert(fsr.render(r,input,dst,assets,false)==0);SDL_RenderFlush(r);
   SDL_Rect probe{260,500,1,1};Uint32 current=0;SDL_RenderReadPixels(r,&probe,SDL_PIXELFORMAT_ARGB8888,&current,4);
   for(int channel:{0,8,16})assert(std::abs(int((current>>channel)&255)-int((0xff406080u>>channel)&255))<4);
   SDL_UpdateTexture(input,nullptr,pixels.data(),1920*4);fsr.invalidate();
   std::cout<<"PASS failed conversion cannot retain stale output\n";
  }
  SDL_Rect native{0,0,1920,1080};assert(fsr.render(r,input,native,assets,true,3)==0);assert(fsr.status().find("Native")!=std::string::npos);
  // SDL's YUYV/NV12 conversion and texture-cache state must survive repeated FSR passes.
  for(Uint32 format:{SDL_PIXELFORMAT_YUY2,SDL_PIXELFORMAT_NV12}){
   auto *yuv=SDL_CreateTexture(r,format,SDL_TEXTUREACCESS_STREAMING,1920,1080);assert(yuv);
   for(int frame=0;frame<2;++frame){
    bool nv=format==SDL_PIXELFORMAT_NV12;int pitch=nv?1920:3840;std::vector<Uint8> bytes(pitch*(nv?1620:1080),128);
    if(nv){for(size_t i=1920*1080;i<bytes.size();i+=2){bytes[i]=frame?200:90;bytes[i+1]=frame?90:200;}}
    else for(size_t i=0;i<bytes.size();i+=4){bytes[i+1]=frame?200:90;bytes[i+3]=frame?90:200;}
    assert(SDL_UpdateTexture(yuv,nullptr,bytes.data(),pitch)==0);
    SDL_RenderCopy(r,yuv,nullptr,&dst);SDL_RenderFlush(r);SDL_Rect probe{260,500,1,1};Uint32 baseline=0,scaled=0;SDL_RenderReadPixels(r,&probe,SDL_PIXELFORMAT_ARGB8888,&baseline,4);
    if(frame)fsr.invalidate();
    assert(fsr.render(r,yuv,dst,assets,frame==0,frame?3:1)==0);SDL_RenderFlush(r);SDL_RenderReadPixels(r,&probe,SDL_PIXELFORMAT_ARGB8888,&scaled,4);
    for(int shift:{0,8,16})assert(std::abs(int((baseline>>shift)&255)-int((scaled>>shift)&255))<4);
   }
   SDL_DestroyTexture(yuv);
  }
  fsr.reset();
 }
 SDL_DestroyTexture(input);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();std::cout<<"PASS FSR output orientation/color, cached redraw, SDL state and native-size bypass\n";
}
