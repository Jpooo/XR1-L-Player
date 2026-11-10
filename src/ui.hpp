#pragma once
#include "core.hpp"
#include <SDL_ttf.h>
#include <array>
#include <unordered_map>
#include <cstdlib>
#include <stdexcept>
#ifdef _WIN32
#include "menu_theme.hpp"
#endif
namespace ui {
inline constexpr int height=650,menuOffset=-72,toolbarOffset=-64;
struct Box {int x,y,w,h;bool contains(int px,int py)const{return px>=x&&px<x+w&&py>=y&&py<y+h;}};
inline constexpr Box fullscreen{895,38,165,42},refresh{568,237,102,30};
inline constexpr std::array<Box,3> devices={Box{60,282,610,66},Box{60,360,610,66},Box{60,438,610,66}};
inline constexpr Box format{142,552,150,42},demo{312,552,208,42};
inline constexpr Box mute{620,552,42,42},volumeHit{670,552,210,42};
inline constexpr int volumeLeft=680,volumeWidth=190,volumeTop=570;
inline constexpr Box primary{40,622,650,56},stop{714,622,346,56},motion{960,552,100,42};
inline constexpr SDL_Color white{243,246,248,255},muted{155,163,172,255},green{155,240,11,255};
inline constexpr SDL_Color surface{24,27,31,255},raised{32,36,41,255},border{48,54,61,255};
enum class Icon {Video,Speaker,Controller,Expand,Refresh,Play,Stop,Chevron,Check};
}
struct Painter {
    SDL_Renderer *r;
    TTF_Font *normal=nullptr,*small=nullptr,*title=nullptr,*heading=nullptr,*tiny=nullptr,*display=nullptr;
    std::string fontPath,assetPath;
    std::unordered_map<std::string,SDL_Texture*> icons;
    std::unordered_map<int,float> transitions;
    SDL_Texture *glowTexture=nullptr;
    bool motion=true;
#ifdef _WIN32
    ui::MenuTheme theme;
    bool applyTheme=true;
    bool setWindowIcon(SDL_Window *window){
        auto path=assetPath+"/branding/xr1-play-"+ui::colorKey(theme.choice)+".bmp";
        auto *surface=SDL_LoadBMP(path.c_str());if(!surface)return false;
        SDL_SetWindowIcon(window,surface);SDL_FreeSurface(surface);return true;
    }
    SDL_Color resolve(SDL_Color c)const{return applyTheme?theme.resolve(c):c;}
#endif
    double frameTime=0,frameDelta=0;
    void frame(double now){frameDelta=frameTime?std::clamp(now-frameTime,0.0,0.1):0;frameTime=now;}
    float animate(int id,bool active){float &value=transitions[id];value=motion?value+((active?1.0f:0.0f)-value)*float(1-std::exp(-frameDelta*16)):(active?1.0f:0.0f);return value;}
    static SDL_Color mix(SDL_Color a,SDL_Color b,float t){auto lerp=[&](Uint8 x,Uint8 y){return static_cast<Uint8>(std::lround(x+(y-x)*t));};return {lerp(a.r,b.r),lerp(a.g,b.g),lerp(a.b,b.b),lerp(a.a,b.a)};}
    float fontScale=0;
    int offsetY=0;
    struct Label {SDL_Texture *texture;float w,h;};
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
    ~Painter(){clearLabels();clearShapes();for(auto &item:icons)SDL_DestroyTexture(item.second);if(glowTexture)SDL_DestroyTexture(glowTexture);if(backdrop)SDL_DestroyTexture(backdrop);for(auto *font:{normal,small,title,heading,tiny,display})if(font)TTF_CloseFont(font);}
    void prepare() {
        int w=0,h=0;SDL_GetRendererOutputSize(r,&w,&h);
        float scale=std::max(0.5f,std::min(w/1100.0f,h/float(ui::height)));
        if(std::abs(scale-fontScale)<0.005f)return;
        std::array<TTF_Font*,6> fonts{};constexpr int sizes[]={18,14,38,23,12,112};
        for(size_t i=0;i<fonts.size();++i){fonts[i]=TTF_OpenFont(fontPath.c_str(),static_cast<int>(std::lround(sizes[i]*scale)));if(!fonts[i]){for(auto *f:fonts)if(f)TTF_CloseFont(f);throw std::runtime_error("Cannot open bundled font: "+std::string(TTF_GetError()));}}
        clearLabels();for(auto *font:{normal,small,title,heading,tiny,display})if(font)TTF_CloseFont(font);
        normal=fonts[0];small=fonts[1];title=fonts[2];heading=fonts[3];tiny=fonts[4];display=fonts[5];
        TTF_SetFontStyle(title,TTF_STYLE_BOLD);TTF_SetFontStyle(heading,TTF_STYLE_BOLD);TTF_SetFontStyle(display,TTF_STYLE_BOLD);fontScale=scale;
    }
    void rect(int x,int y,int w,int h,SDL_Color c){
#ifdef _WIN32
        c=resolve(c);
#endif
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
    void round(ui::Box b,float radius,SDL_Color color){
#ifdef _WIN32
        color=resolve(color);
#endif
        b.y+=offsetY;
        if(b.w<=0||b.h<=0)return;
        radius=std::min(radius,std::min(b.w,b.h)/2.0f);
        std::string key=std::to_string(b.w)+":"+std::to_string(b.h)+":"+std::to_string(radius);
        auto found=shapes.find(key);
        if(found==shapes.end()){
            auto *texture=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,b.w,b.h);if(!texture)return;
            auto *format=SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);if(!format){SDL_DestroyTexture(texture);return;}
            Uint32 solid=SDL_MapRGBA(format,255,255,255,255);std::vector<Uint32> pixels(static_cast<size_t>(b.w)*b.h,solid);
            // Cache one antialiased mask per shape/color. One texture draw avoids
            // seams where separately rounded primitives meet at fractional DPI.
            int band=static_cast<int>(std::ceil(radius+1));
            for(int y=0;y<b.h;++y){if(y>=band&&y<b.h-band)continue;
                for(int x=0;x<b.w;++x){if(x>=band&&x<b.w-band)continue;
                    float dx=std::max(std::abs(x+.5f-b.w/2.0f)-(b.w/2.0f-radius),0.0f);
                    float dy=std::max(std::abs(y+.5f-b.h/2.0f)-(b.h/2.0f-radius),0.0f);
                    float coverage=std::clamp(.5f-(std::hypot(dx,dy)-radius),0.0f,1.0f);
                    pixels[static_cast<size_t>(y)*b.w+x]=SDL_MapRGBA(format,255,255,255,static_cast<Uint8>(std::lround(255*coverage)));
                }
            }
            SDL_UpdateTexture(texture,nullptr,pixels.data(),b.w*4);SDL_FreeFormat(format);SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
            if(shapes.size()>=96)clearShapes();
            found=shapes.emplace(std::move(key),texture).first;
        }
        SDL_SetTextureColorMod(found->second,color.r,color.g,color.b);SDL_SetTextureAlphaMod(found->second,color.a);
        SDL_FRect dst{float(b.x),float(b.y),float(b.w),float(b.h)};SDL_RenderCopyF(r,found->second,nullptr,&dst);
    }
    void card(ui::Box b,SDL_Color fill,SDL_Color stroke,int radius=14){round(b,float(radius),stroke);round({b.x+1,b.y+1,b.w-2,b.h-2},float(radius-1),fill);}
    void line(float x1,float y1,float x2,float y2,SDL_Color c,float width=2){
#ifdef _WIN32
        c=resolve(c);
#endif
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
#ifdef _WIN32
        color=resolve(color);
#endif
        dst.y+=offsetY;
        if(!glowTexture){
            constexpr int size=192;std::vector<Uint32> pixels(size*size);auto *format=SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
            for(int y=0;y<size;++y)for(int x=0;x<size;++x){float distance=std::hypot((x+.5f-size/2)/(size/2),(y+.5f-size/2)/(size/2));float a=std::pow(std::max(0.0f,1-distance),2);pixels[y*size+x]=SDL_MapRGBA(format,255,255,255,static_cast<Uint8>(a*255));}
            glowTexture=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,size,size);if(!glowTexture){SDL_FreeFormat(format);return;}
            SDL_UpdateTexture(glowTexture,nullptr,pixels.data(),size*4);SDL_FreeFormat(format);SDL_SetTextureBlendMode(glowTexture,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(glowTexture,SDL_ScaleModeLinear);
        }
        SDL_SetTextureColorMod(glowTexture,color.r,color.g,color.b);SDL_SetTextureAlphaMod(glowTexture,color.a);SDL_RenderCopyF(r,glowTexture,nullptr,&dst);
    }
    void icon(ui::Icon type,int x,int y,int size,SDL_Color c,bool secondary=false,double angle=0){
#ifdef _WIN32
        c=resolve(c);
#endif
        y+=offsetY;
        static const char *names[]={"monitor","volume-2","gamepad-2","maximize","refresh-cw","play","square","chevron-right","check"};
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
    void text(const std::string &s,int x,int y,SDL_Color color,TTF_Font *font=nullptr,int wrap=0){
#ifdef _WIN32
        color=resolve(color);
#endif
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
            Label label{texture,surface->w/fontScale,surface->h/fontScale};SDL_FreeSurface(surface);if(!texture)return;
            SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);if(labels.size()>=256)clearLabels();found=labels.emplace(std::move(key),label).first;
        }
        SDL_FRect dst{float(x),float(y),found->second.w,found->second.h};SDL_RenderCopyF(r,found->second.texture,nullptr,&dst);
    }
    void fit(std::string s,int x,int y,int width,SDL_Color color,TTF_Font *font=nullptr){
        if(!font)font=normal;
        int w=0,h=0;TTF_SizeUTF8(font,s.c_str(),&w,&h);
        if(w/fontScale>width){do{size_t last=s.size()-1;while(last>0&&(static_cast<unsigned char>(s[last])&0xc0)==0x80)--last;s.resize(last);TTF_SizeUTF8(font,(s+"…").c_str(),&w,&h);}while(!s.empty()&&w/fontScale>width);s+="…";}
        text(s,x,y,color,font);
    }
    void button(ui::Box b,const std::string &label,bool hovered,bool primary=false,TTF_Font *font=nullptr){
        if(!font)font=normal;
        float energy=animate(b.x*1000+b.y,hovered);int lift=int(std::lround(energy*2));
        b.y-=lift;
#ifdef _WIN32
        auto accent=theme.accent();auto glow=accent;glow.a=static_cast<Uint8>(8+energy*10);
        if(primary)round({b.x-2,b.y+3,b.w+4,b.h+3},10,glow);
        auto fill=primary?mix(accent,ui::white,energy*.10f):mix(ui::raised,{44,49,57,255},energy);
        card(b,fill,primary?accent:mix(ui::border,accent,energy*.55f),9);
        int w=0,h=0;TTF_SizeUTF8(font,label.c_str(),&w,&h);text(label,b.x+(b.w-int(w/fontScale))/2,b.y+(b.h-int(h/fontScale))/2,primary?theme.ink():ui::white,font);
#else
        if(primary){round({b.x-3,b.y+3,b.w+6,b.h+5},14,{155,240,11,static_cast<Uint8>(12+energy*20)});}
        auto fill=primary?mix(ui::green,{185,255,64,255},energy):mix(ui::raised,{48,57,65,255},energy);
        card(b,fill,mix(primary?ui::green:ui::border,ui::green,energy),10);
        int w=0,h=0;TTF_SizeUTF8(font,label.c_str(),&w,&h);text(label,b.x+(b.w-int(w/fontScale))/2,b.y+(b.h-int(h/fontScale))/2,primary?SDL_Color{14,24,5,255}:ui::white,font);
#endif
    }
};
