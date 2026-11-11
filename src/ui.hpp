#pragma once
#include "core.hpp"
#include <SDL_ttf.h>
#include <array>
#include <unordered_map>
#include <cstdlib>
#include <stdexcept>
#include <limits>
#include <filesystem>
#include "menu_theme.hpp"
#include "menu_motion.hpp"
#include "window_icon.hpp"
namespace ui {
inline constexpr int height=650,toolbarOffset=-64;
struct Box {int x,y,w,h;bool contains(int px,int py)const{return px>=x&&px<x+w&&py>=y&&py<y+h;}};
enum class Screen {Main,Settings,Preferences};
inline constexpr std::array<Box,3> navigation={Box{48,190,350,108},Box{48,322,350,80},Box{48,426,350,80}};
inline constexpr Box back{920,32,140,42},refresh{40,506,160,44},fullscreen{380,506,180,44};
inline constexpr std::array<Box,3> devices={Box{60,218,445,66},Box{595,218,445,66},Box{595,304,445,66}};
inline constexpr Box format{60,304,445,56},demo{60,386,210,44};
// The video is clipped to this panel contour; uniform UV scaling preserves aspect.
inline constexpr SDL_Rect mainPreview{452,174,606,344};
inline constexpr std::array<SDL_FPoint,14> mainPanelContour={SDL_FPoint{452,244},SDL_FPoint{462,209},SDL_FPoint{486,185},SDL_FPoint{637,185},SDL_FPoint{654,174},SDL_FPoint{1026,174},SDL_FPoint{1058,206},SDL_FPoint{1058,466},SDL_FPoint{1032,506},SDL_FPoint{866,506},SDL_FPoint{850,518},SDL_FPoint{817,515},SDL_FPoint{475,515},SDL_FPoint{452,473}};
inline constexpr std::array<int,36> mainPanelTriangles={13,0,1,13,1,2,13,2,3,13,3,4,13,4,5,13,5,6,13,6,7,13,7,8,13,8,9,13,9,10,13,10,11,11,12,13};
inline constexpr Box mute{595,398,44,44},volumeHit{640,398,280,44};
inline constexpr int volumeLeft=650,volumeWidth=260,volumeTop=418;
inline constexpr Box playbackToolbar{28,644,1044,60},playbackMain{42,654,134,40},playbackSettings{188,654,112,40};
inline constexpr Box playbackMute{312,654,86,40},playbackVolumeHit{408,650,216,44},playbackStop{710,654,92,40},playbackFullscreen{820,654,214,40};
inline constexpr int playbackVolumeLeft=418,playbackVolumeWidth=196,playbackVolumeTop=670;
inline constexpr Box primary{845,506,215,44},stop{660,506,160,44},motion{40,376,500,92},preferenceFullscreen{575,376,485,92};
inline constexpr std::array<Box,3> themes={Box{40,178,320,154},Box{390,178,320,154},Box{740,178,320,154}};
inline constexpr std::array<Box,12> settingControls={devices[0],format,demo,devices[1],devices[2],mute,volumeHit,refresh,fullscreen,primary,stop,back};
inline constexpr std::array<Box,6> preferenceControls={themes[0],themes[1],themes[2],motion,preferenceFullscreen,back};
inline constexpr float logoX=480,logoY=75,logoSize=540;
inline constexpr SDL_Color white{243,246,248,255},muted{155,163,172,255},green{155,240,11,255};
inline constexpr SDL_Color surface{24,27,31,255},raised{32,36,41,255},border{48,54,61,255};
enum class Icon {Video,Speaker,Capture,Expand,Refresh,Play,Stop,Chevron,Check};
}
struct Painter {
    SDL_Renderer *r;
    TTF_Font *normal=nullptr,*small=nullptr,*title=nullptr,*heading=nullptr,*tiny=nullptr,*display=nullptr;
    std::string fontPath,assetPath;
    std::unordered_map<std::string,SDL_Texture*> icons;
    std::unordered_map<int,float> transitions;
    SDL_Texture *glowTexture=nullptr;
    bool motion=true;
    ui::MenuTheme theme;
    bool applyTheme=true;
    ui::NativeWindowIcons nativeIcons;
    bool setWindowIcon(SDL_Window *window){
        if(nativeIcons.set(window,theme.choice))return true;
        auto path=assetPath+"/branding/xr1-play-"+ui::colorKey(theme.choice)+".bmp";
        auto *surface=SDL_LoadBMP(path.c_str());if(!surface)return false;
        SDL_SetWindowIcon(window,surface);SDL_FreeSurface(surface);return true;
    }
    SDL_Color resolve(SDL_Color c)const{return applyTheme?theme.resolve(c):c;}
    double frameTime=0,frameDelta=0;
    void frame(double now){frameDelta=frameTime?std::clamp(now-frameTime,0.0,0.1):0;frameTime=now;}
    float animate(int id,bool active){float &value=transitions[id];value=motion?value+((active?1.0f:0.0f)-value)*float(1-std::exp(-frameDelta*16)):(active?1.0f:0.0f);return value;}
    static SDL_Color mix(SDL_Color a,SDL_Color b,float t){auto lerp=[&](Uint8 x,Uint8 y){return static_cast<Uint8>(std::lround(x+(y-x)*t));};return {lerp(a.r,b.r),lerp(a.g,b.g),lerp(a.b,b.b),lerp(a.a,b.a)};}
    float fontScale=0;
    int offsetY=0;
    struct Label {SDL_Texture *texture;int w,h;};
    std::unordered_map<std::string,Label> labels;
    std::unordered_map<std::string,SDL_Texture*> shapes;
    void clearShapes(){for(auto &item:shapes)SDL_DestroyTexture(item.second);shapes.clear();}
    SDL_Texture *backdrop=nullptr;
    int backdropW=0,backdropH=0;
    uint64_t backdropColors=0;
    explicit Painter(SDL_Renderer *renderer):r(renderer) {
        const char *root=std::getenv("XR1_ASSET_DIR");
        if(root&&*root)assetPath=root;else {char *base=SDL_GetBasePath();assetPath=std::string(base?base:"")+"assets";SDL_free(base);if(!std::filesystem::exists(std::filesystem::path(assetPath)/"DejaVuSans.ttf"))assetPath="assets";}
        fontPath=assetPath+"/DejaVuSans.ttf";prepare();
    }
    void clearLabels(){for(auto &item:labels)SDL_DestroyTexture(item.second.texture);labels.clear();}
    void resetTextures(){
        clearLabels();clearShapes();
        for(auto &item:icons)SDL_DestroyTexture(item.second);
        icons.clear();
        if(glowTexture)SDL_DestroyTexture(glowTexture);
        if(backdrop)SDL_DestroyTexture(backdrop);
        glowTexture=backdrop=nullptr;backdropW=backdropH=0;backdropColors=0;
    }
    ~Painter(){resetTextures();for(auto *font:{normal,small,title,heading,tiny,display})if(font)TTF_CloseFont(font);}
    void prepare() {
        int w=0,h=0;SDL_GetRendererOutputSize(r,&w,&h);
        float scale=std::max(0.5f,std::min(w/1100.0f,h/float(ui::height)));
        if(std::abs(scale-fontScale)<0.005f)return;
        std::array<TTF_Font*,6> fonts{};constexpr int sizes[]={18,14,38,23,12,112};
        for(size_t i=0;i<fonts.size();++i){
            const auto path=(i==2||i==3||i==5)?assetPath+"/fonts/Inter-SemiBold.ttf":fontPath;
            fonts[i]=TTF_OpenFont(path.c_str(),static_cast<int>(std::lround(sizes[i]*scale)));
            if(!fonts[i]){for(auto *f:fonts)if(f)TTF_CloseFont(f);throw std::runtime_error("Cannot open bundled font: "+std::string(TTF_GetError()));}
        }
        for(auto *font:fonts)TTF_SetFontHinting(font,TTF_HINTING_LIGHT);
        clearLabels();clearShapes();for(auto *font:{normal,small,title,heading,tiny,display})if(font)TTF_CloseFont(font);
        normal=fonts[0];small=fonts[1];title=fonts[2];heading=fonts[3];tiny=fonts[4];display=fonts[5];
        fontScale=scale; // Headings use Inter's designed semibold face, without synthetic bold.
    }
    void rect(int x,int y,int w,int h,SDL_Color c){
        c=resolve(c);
        y+=offsetY;SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);SDL_Rect a{x,y,w,h};SDL_RenderFillRect(r,&a);}
    void gradient(ui::Box b,SDL_Color top,SDL_Color bottom){
        b.y+=offsetY;
        auto rgba=[](SDL_Color c){return (uint64_t(c.r)<<24)|(uint64_t(c.g)<<16)|(uint64_t(c.b)<<8)|c.a;};
        uint64_t colors=(rgba(top)<<32)|rgba(bottom);
        if(!backdrop||backdropW!=b.w||backdropH!=b.h||backdropColors!=colors){
            if(backdrop)SDL_DestroyTexture(backdrop);
            backdrop=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,b.w,b.h);
            if(!backdrop){rect(b.x,b.y,b.w,b.h,bottom);return;}
            auto *format=SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);std::vector<Uint32> pixels(static_cast<size_t>(b.w)*b.h);
            for(int y=0;y<b.h;++y){float t=float(y)/std::max(1,b.h-1);auto blend=[&](Uint8 a,Uint8 z){return static_cast<Uint8>(a+(z-a)*t);};Uint32 pixel=SDL_MapRGBA(format,blend(top.r,bottom.r),blend(top.g,bottom.g),blend(top.b,bottom.b),blend(top.a,bottom.a));std::fill_n(pixels.begin()+static_cast<size_t>(y)*b.w,b.w,pixel);}
            SDL_UpdateTexture(backdrop,nullptr,pixels.data(),b.w*4);SDL_FreeFormat(format);SDL_SetTextureScaleMode(backdrop,SDL_ScaleModeLinear);
            backdropW=b.w;backdropH=b.h;backdropColors=colors;
        }
        SDL_Rect dst{b.x,b.y,b.w,b.h};SDL_RenderCopy(r,backdrop,nullptr,&dst);
    }
    void round(ui::Box b,float radius,SDL_Color color,float shiftY=0,double angle=0,const SDL_FPoint *center=nullptr){
        color=resolve(color);
        b.y+=offsetY;
        if(b.w<=0||b.h<=0)return;
        radius=std::min(radius,std::min(b.w,b.h)/2.0f);
        const int pixelW=std::max(1,int(std::lround(b.w*fontScale))),pixelH=std::max(1,int(std::lround(b.h*fontScale)));
        const float pixelRadius=radius*fontScale;
        std::string key=std::to_string(pixelW)+":"+std::to_string(pixelH)+":"+std::to_string(pixelRadius);
        auto found=shapes.find(key);
        if(found==shapes.end()){
            auto *texture=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,pixelW,pixelH);if(!texture)return;
            auto *format=SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);if(!format){SDL_DestroyTexture(texture);return;}
            Uint32 solid=SDL_MapRGBA(format,255,255,255,255);std::vector<Uint32> pixels(static_cast<size_t>(pixelW)*pixelH,solid);
            // Cache one mask at the drawable pixel density. One texture draw avoids
            // seams where separately rounded primitives meet at fractional DPI.
            int band=static_cast<int>(std::ceil(pixelRadius+1));
            for(int y=0;y<pixelH;++y){if(y>=band&&y<pixelH-band)continue;
                for(int x=0;x<pixelW;++x){if(x>=band&&x<pixelW-band)continue;
                    float dx=std::max(std::abs(x+.5f-pixelW/2.0f)-(pixelW/2.0f-pixelRadius),0.0f);
                    float dy=std::max(std::abs(y+.5f-pixelH/2.0f)-(pixelH/2.0f-pixelRadius),0.0f);
                    float coverage=std::clamp(.5f-(std::hypot(dx,dy)-pixelRadius),0.0f,1.0f);
                    pixels[static_cast<size_t>(y)*pixelW+x]=SDL_MapRGBA(format,255,255,255,static_cast<Uint8>(std::lround(255*coverage)));
                }
            }
            SDL_UpdateTexture(texture,nullptr,pixels.data(),pixelW*4);SDL_FreeFormat(format);SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
            if(shapes.size()>=96)clearShapes();
            found=shapes.emplace(std::move(key),texture).first;
        }
        SDL_SetTextureColorMod(found->second,color.r,color.g,color.b);SDL_SetTextureAlphaMod(found->second,color.a);
        SDL_FRect dst{float(b.x),float(b.y)+shiftY,float(b.w),float(b.h)};
        if(angle||center)SDL_RenderCopyExF(r,found->second,nullptr,&dst,angle,center,SDL_FLIP_NONE);else SDL_RenderCopyF(r,found->second,nullptr,&dst);
    }
    void card(ui::Box b,SDL_Color fill,SDL_Color stroke,int radius=14,float shiftY=0){round(b,float(radius),stroke,shiftY);round({b.x+1,b.y+1,b.w-2,b.h-2},float(radius-1),fill,shiftY);}
    void line(float x1,float y1,float x2,float y2,SDL_Color c,float width=2){
        c=resolve(c);
        y1+=offsetY;y2+=offsetY;
        float length=std::hypot(x2-x1,y2-y1);if(length<0.001f)return;
        float ux=(x2-x1)/length,uy=(y2-y1)/length,nx=-uy,ny=ux;
        SDL_Color clear=c;clear.a=0;std::array<SDL_Vertex,8> v{};
        for(int i=0;i<4;++i){bool end=i==1||i==2;float sign=i<2?1.0f:-1.0f;
            float x=end?x2:x1,y=end?y2:y1;
            v[i]={{x+nx*sign*width/2,y+ny*sign*width/2},c,{}};
            v[i+4]={{x+ux*(end?.5f:-.5f)+nx*sign*(width/2+.7f),y+uy*(end?.5f:-.5f)+ny*sign*(width/2+.7f)},clear,{}};
        }
        int indices[]={0,1,2,0,2,3,0,4,5,0,5,1,1,5,6,1,6,2,2,6,7,2,7,3,3,7,4,3,4,0};SDL_RenderGeometry(r,nullptr,v.data(),8,indices,30);
    }
    void glow(SDL_FRect dst,SDL_Color color){
        color=resolve(color);
        dst.y+=offsetY;
        if(!glowTexture){
            constexpr int size=192;std::vector<Uint32> pixels(size*size);auto *format=SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
            for(int y=0;y<size;++y)for(int x=0;x<size;++x){float distance=std::hypot((x+.5f-size/2)/(size/2),(y+.5f-size/2)/(size/2));float a=std::pow(std::max(0.0f,1-distance),2);pixels[y*size+x]=SDL_MapRGBA(format,255,255,255,static_cast<Uint8>(a*255));}
            glowTexture=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,size,size);if(!glowTexture){SDL_FreeFormat(format);return;}
            SDL_UpdateTexture(glowTexture,nullptr,pixels.data(),size*4);SDL_FreeFormat(format);SDL_SetTextureBlendMode(glowTexture,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(glowTexture,SDL_ScaleModeLinear);
        }
        SDL_SetTextureColorMod(glowTexture,color.r,color.g,color.b);SDL_SetTextureAlphaMod(glowTexture,color.a);SDL_RenderCopyF(r,glowTexture,nullptr,&dst);
    }
    void icon(ui::Icon type,float x,float y,float size,SDL_Color c,bool secondary=false,double angle=0){
        c=resolve(c);
        y+=offsetY;
        static const char *names[]={"monitor","volume-2","xr1-lite","maximize","refresh-cw","play","square","chevron-right","check"};
        std::string name=type==ui::Icon::Speaker&&secondary?"volume-x":names[static_cast<int>(type)];
        auto found=icons.find(name);
        if(found==icons.end()){
            auto *surface=SDL_LoadBMP((assetPath+"/icons/"+name+".bmp").c_str());
            if(!surface)throw std::runtime_error("Cannot load bundled icon "+name+": "+SDL_GetError());
            auto *texture=SDL_CreateTextureFromSurface(r,surface);SDL_FreeSurface(surface);if(!texture)throw std::runtime_error(SDL_GetError());
            SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);found=icons.emplace(name,texture).first;
        }
        SDL_SetTextureColorMod(found->second,c.r,c.g,c.b);SDL_SetTextureAlphaMod(found->second,c.a);
        SDL_FRect dst{float(x),float(y),float(size),float(size)};
        if(type==ui::Icon::Chevron&&secondary)angle=180;
        SDL_RenderCopyExF(r,found->second,nullptr,&dst,angle,nullptr,SDL_FLIP_NONE);
    }
    void text(const std::string &s,float x,float y,SDL_Color color,TTF_Font *font=nullptr,int wrap=0){
        color=resolve(color);
        y+=offsetY;
        if(s.empty())return;
        if(!font)font=normal;
        char id=font==small?'s':font==title?'t':font==heading?'h':font==tiny?'c':font==display?'d':'n';
        uint32_t rgba=(uint32_t(color.r)<<24)|(uint32_t(color.g)<<16)|(uint32_t(color.b)<<8)|color.a;
        std::string key=std::string(1,id)+std::to_string(rgba)+":"+std::to_string(wrap)+":"+s;
        auto found=labels.find(key);
        if(found==labels.end()){
            auto *surface=wrap?TTF_RenderUTF8_Blended_Wrapped(font,s.c_str(),color,static_cast<Uint32>(wrap*fontScale)):TTF_RenderUTF8_Blended(font,s.c_str(),color);
            if(!surface)return;
            auto *texture=SDL_CreateTextureFromSurface(r,surface);
            Label label{texture,surface->w,surface->h};SDL_FreeSurface(surface);if(!texture)return;
            SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);if(labels.size()>=256)clearLabels();found=labels.emplace(std::move(key),label).first;
        }
        // Glyphs were rasterized at the drawable's physical resolution. Align
        // their origin to that pixel grid instead of filtering between pixels.
        float sx=1,sy=1;SDL_RenderGetScale(r,&sx,&sy);
        // Nudge the inverse transform by one float ULP: SDL's software
        // backend truncates a 230.99998-pixel rectangle to 230 pixels.
        auto logicalPixel=[](float pixel,float scale){return std::nextafter(pixel/scale,std::numeric_limits<float>::infinity());};
        SDL_FRect dst{logicalPixel(std::round(x*sx),sx),logicalPixel(std::round(y*sy),sy),logicalPixel(float(found->second.w),sx),logicalPixel(float(found->second.h),sy)};SDL_RenderCopyF(r,found->second.texture,nullptr,&dst);
    }
    void fit(std::string s,int x,int y,int width,SDL_Color color,TTF_Font *font=nullptr){
        if(!font)font=normal;
        int w=0,h=0;TTF_SizeUTF8(font,s.c_str(),&w,&h);
        if(w/fontScale>width){do{size_t last=s.size()-1;while(last>0&&(static_cast<unsigned char>(s[last])&0xc0)==0x80)--last;s.resize(last);TTF_SizeUTF8(font,(s+"…").c_str(),&w,&h);}while(!s.empty()&&w/fontScale>width);s+="…";}
        text(s,x,y,color,font);
    }
    void button(ui::Box b,const std::string &label,bool hovered,bool primary=false,TTF_Font *font=nullptr){
        if(!font)font=normal;
        float energy=animate(b.x*1000+b.y,hovered);float lift=energy*2;
        auto accent=theme.accent();auto glow=accent;glow.a=static_cast<Uint8>(8+energy*10);
        if(primary)round({b.x-2,b.y+3,b.w+4,b.h+3},10,glow,-lift);
        auto fill=primary?mix(accent,ui::white,energy*.10f):mix(ui::raised,{44,49,57,255},energy);
        card(b,fill,primary?accent:mix(ui::border,accent,energy*.55f),9,-lift);
        int w=0,h=0;TTF_SizeUTF8(font,label.c_str(),&w,&h);text(label,b.x+(b.w-int(w/fontScale))/2,b.y+(b.h-int(h/fontScale))/2-lift,primary?theme.ink():ui::white,font);
    }
};
