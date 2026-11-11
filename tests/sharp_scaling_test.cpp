#include <SDL.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>
static int allocations=0;static bool failAllocation=false;
static SDL_Texture *measuredCreate(SDL_Renderer *r,Uint32 format,int access,int w,int h){++allocations;if(failAllocation){SDL_SetError("Injected target allocation failure");return nullptr;}return SDL_CreateTexture(r,format,access,w,h);}
#define SDL_CreateTexture measuredCreate
#include "../src/sharp_scaling.hpp"
#undef SDL_CreateTexture
struct RenderState {
 SDL_Texture *target;int lw=0,lh=0;SDL_Rect viewport{},clip{};float sx=0,sy=0;SDL_bool clipping,integer;Uint8 r,g,b,a;SDL_BlendMode blend;
 explicit RenderState(SDL_Renderer *renderer):target(SDL_GetRenderTarget(renderer)),clipping(SDL_RenderIsClipEnabled(renderer)),integer(SDL_RenderGetIntegerScale(renderer)){SDL_RenderGetLogicalSize(renderer,&lw,&lh);SDL_RenderGetViewport(renderer,&viewport);SDL_RenderGetClipRect(renderer,&clip);SDL_RenderGetScale(renderer,&sx,&sy);SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a);SDL_GetRenderDrawBlendMode(renderer,&blend);}
 void same(SDL_Renderer *renderer)const{RenderState other(renderer);assert(target==other.target&&lw==other.lw&&lh==other.lh&&clipping==other.clipping&&integer==other.integer);assert(std::memcmp(&viewport,&other.viewport,sizeof(viewport))==0&&std::memcmp(&clip,&other.clip,sizeof(clip))==0);assert(std::abs(sx-other.sx)<.00001f&&std::abs(sy-other.sy)<.00001f);assert(r==other.r&&g==other.g&&b==other.b&&a==other.a&&blend==other.blend);}
};
static std::vector<Uint32> read(SDL_Renderer *r,int width,int height){std::vector<Uint32> pixels(size_t(width)*height);assert(SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,pixels.data(),width*4)==0);return pixels;}
static void clear(SDL_Renderer *r){SDL_SetRenderDrawColor(r,0,0,0,255);SDL_RenderClear(r);}
static int byte(double v){return int(std::round(std::max(0.,std::min(255.,v))));}
struct Yuv{double y,u,v;};
static Yuv yuv(Uint32 p){double r=(p>>16)&255,g=(p>>8)&255,b=p&255;return {16+.182586*r+.614231*g+.062007*b,128-.100644*r-.338572*g+.439216*b,128+.439216*r-.398942*g-.040274*b};}
int main(){
 SDL_SetMainReady();bool gpu=std::getenv("XR1_SHARP_GPU")!=nullptr;
 if(!gpu)SDL_setenv("SDL_VIDEODRIVER","dummy",1);
 assert(SDL_Init(SDL_INIT_VIDEO)==0);auto *window=SDL_CreateWindow("Sharp fullscreen test",0,0,640,360,SDL_WINDOW_HIDDEN);assert(window);
 auto *r=SDL_CreateRenderer(window,-1,(gpu?SDL_RENDERER_ACCELERATED:SDL_RENDERER_SOFTWARE)|SDL_RENDERER_TARGETTEXTURE);assert(r);SDL_RendererInfo info{};SDL_GetRendererInfo(r,&info);std::cout<<"Renderer "<<info.name<<'\n';
 {
 std::vector<Uint32> rgb(1920*1080,0xff101820);
 for(int y=100;y<210;++y)for(int x=100;x<600;++x)rgb[y*1920+x]=0xffff3030;
 for(int y=250;y<360;++y)for(int x=100;x<600;++x)rgb[y*1920+x]=0xff3080ff;
 for(int y=450;y<600;++y)for(int x=600;x<1100;++x)rgb[y*1920+x]=((x/3+y/7)&1)?0xffeeeeee:0xff13191f;
 auto *source=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,1920,1080);assert(source);SDL_UpdateTexture(source,nullptr,rgb.data(),1920*4);SDL_SetTextureBlendMode(source,SDL_BLENDMODE_NONE);SDL_SetTextureScaleMode(source,SDL_ScaleModeNearest);
 auto *output=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,2560,1440);auto *reference=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,3840,2160);assert(output&&reference);SDL_SetTextureBlendMode(reference,SDL_BLENDMODE_NONE);SDL_SetTextureScaleMode(reference,SDL_ScaleModeLinear);
 SDL_SetRenderTarget(r,output);clear(r);SDL_Rect native{0,0,1920,1080},fractional{0,0,2560,1440};
 SharpFullscreenScaling helper(false);int before=allocations;
 assert(helper.render(r,source,native)==0);assert(allocations==before);auto nativePixels=read(r,2560,1440);for(int y=0;y<1080;++y)for(int x=0;x<1920;++x)assert(nativePixels[y*2560+x]==rgb[y*1920+x]);
 SDL_SetRenderTarget(r,reference);clear(r);SDL_RenderCopy(r,source,nullptr,nullptr);SDL_SetRenderTarget(r,output);clear(r);SDL_RenderCopy(r,reference,nullptr,nullptr);auto expected=read(r,2560,1440);
 clear(r);RenderState base(r);assert(helper.render(r,source,fractional)==0);base.same(r);SDL_ScaleMode filter;SDL_GetTextureScaleMode(source,&filter);assert(filter==SDL_ScaleModeNearest);assert(read(r,2560,1440)==expected);
 int warm=allocations;for(int i=0;i<10;++i)assert(helper.render(r,source,fractional)==0);assert(allocations==warm);
 SDL_Rect integer{0,0,3840,2160};assert(helper.render(r,source,integer)==0);assert(allocations==warm);
 // A deliberately non-default caller state must survive the intermediate pass.
 SDL_RenderSetIntegerScale(r,SDL_TRUE);SDL_RenderSetLogicalSize(r,1000,600);SDL_RenderSetScale(r,1.5f,1.25f);SDL_Rect viewport{14,18,900,500},clip{30,40,700,300};SDL_RenderSetViewport(r,&viewport);SDL_RenderSetClipRect(r,&clip);SDL_SetRenderDrawColor(r,12,34,56,78);SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_ADD);RenderState styled(r);assert(helper.render(r,source,fractional)==0);styled.same(r);
 SDL_RenderSetIntegerScale(r,SDL_FALSE);SDL_RenderSetLogicalSize(r,0,0);SDL_RenderSetScale(r,1,1);SDL_RenderSetViewport(r,nullptr);SDL_RenderSetClipRect(r,nullptr);SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
 helper.reset();failAllocation=true;before=allocations;RenderState failedState(r);assert(helper.render(r,source,fractional)==0);failedState.same(r);assert(allocations>before);int failed=allocations;failAllocation=false;for(int i=0;i<5;++i)assert(helper.render(r,source,fractional)==0);assert(allocations==failed);helper.reset();assert(helper.render(r,source,fractional)==0);assert(allocations>failed);
 if(!gpu){SharpFullscreenScaling guarded;before=allocations;assert(guarded.render(r,source,fractional)==0);assert(allocations==before);}
 // Solid red/blue and background samples catch the NV12 FBO gray-state bug.
 std::vector<Uint8> packed(1920*1080*2),planar(1920*1080*3/2);
 for(int y=0;y<1080;++y)for(int x=0;x<1920;x+=2){auto a=yuv(rgb[y*1920+x]),b=yuv(rgb[y*1920+x+1]);size_t i=size_t(y*1920+x)*2;packed[i]=byte(a.y);packed[i+1]=byte((a.u+b.u)/2);packed[i+2]=byte(b.y);packed[i+3]=byte((a.v+b.v)/2);}
 for(int y=0;y<1080;++y)for(int x=0;x<1920;++x)planar[y*1920+x]=byte(yuv(rgb[y*1920+x]).y);
 for(int y=0;y<1080;y+=2)for(int x=0;x<1920;x+=2){double u=0,v=0;for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx){auto p=yuv(rgb[(y+dy)*1920+x+dx]);u+=p.u;v+=p.v;}size_t i=1920*1080+(y/2)*1920+x;planar[i]=byte(u/4);planar[i+1]=byte(v/4);}
 SDL_SetYUVConversionMode(SDL_YUV_CONVERSION_BT709);
 for(auto format:{SDL_PIXELFORMAT_YUY2,SDL_PIXELFORMAT_NV12}){
  auto *video=SDL_CreateTexture(r,format,SDL_TEXTUREACCESS_STATIC,1920,1080);assert(video);SDL_UpdateTexture(video,nullptr,format==SDL_PIXELFORMAT_NV12?planar.data():packed.data(),format==SDL_PIXELFORMAT_NV12?1920:3840);SDL_SetTextureScaleMode(video,SDL_ScaleModeLinear);SDL_SetTextureBlendMode(video,SDL_BLENDMODE_NONE);helper.reset();
  clear(r);SDL_RenderCopy(r,video,nullptr,&fractional);auto direct=read(r,2560,1440);clear(r);assert(helper.render(r,video,fractional)==0);auto sharp=read(r,2560,1440);
  for(auto index:{size_t(200)*2560+300,size_t(400)*2560+300,size_t(1000)*2560+1600}){auto a=sharp[index],b=direct[index];for(int shift:{0,8,16})assert(std::abs(int((a>>shift)&255)-int((b>>shift)&255))<=3);}
  auto background=sharp[size_t(1000)*2560+1600];assert(((background>>16)&255)<40&&((background>>8)&255)<40&&(background&255)<40);
  SDL_GetTextureScaleMode(video,&filter);assert(filter==SDL_ScaleModeLinear);int count=allocations;for(int i=0;i<3;++i)assert(helper.render(r,video,fractional)==0);assert(count==allocations);SDL_DestroyTexture(video);
 }
 helper.reset();SDL_SetRenderTarget(r,nullptr);SDL_DestroyTexture(source);SDL_DestroyTexture(output);SDL_DestroyTexture(reference);
 }
 SDL_DestroyRenderer(r);SDL_DestroyWindow(window);SDL_Quit();std::cout<<"PASS native bypass, sharp reference, integer bypass, renderer/filter restoration, fallback/retry cache, YUYV/NV12 colors\n";
}
