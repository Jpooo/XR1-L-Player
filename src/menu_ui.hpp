#pragma once
#include "ui.hpp"
namespace ui {
inline const char *themeName(ConsoleColor c){return c==ConsoleColor::PlayStation?"Electric Blue":c==ConsoleColor::Nintendo?"Crimson Red":"Neon Green";}
inline SDL_Color opacity(SDL_Color c,Uint8 a){c.a=a;return c;}
// A pair of antialiased ribbons. Static menu decoration, never drawn over video.
inline void curve(Painter &p,SDL_FPoint a,SDL_FPoint b,SDL_FPoint c,SDL_FPoint d,SDL_Color color,float width){
    constexpr int steps=64;
    for(int layer=0;layer<2;++layer){
        std::array<SDL_Vertex,(steps+1)*4> vertices{};std::array<int,steps*18> indices{};
        float half=layer?width*.5f:width*4;
        for(int i=0;i<=steps;++i){
            float t=float(i)/steps,u=1-t;
            float x=u*u*u*a.x+3*u*u*t*b.x+3*u*t*t*c.x+t*t*t*d.x;
            float y=u*u*u*a.y+3*u*u*t*b.y+3*u*t*t*c.y+t*t*t*d.y;
            float dx=3*u*u*(b.x-a.x)+6*u*t*(c.x-b.x)+3*t*t*(d.x-c.x);
            float dy=3*u*u*(b.y-a.y)+6*u*t*(c.y-b.y)+3*t*t*(d.y-c.y);
            float n=std::max(1.0f,std::hypot(dx,dy)),nx=-dy/n*half,ny=dx/n*half;
            auto ink=color;ink.a=Uint8(color.a*(layer?1:.07f)*std::sin(t*3.14159265f));
            auto clear=ink;clear.a=0;float ox=-dy/n,oy=dx/n;
            vertices[i*4]={{x+nx+ox,y+ny+oy},clear,{}};vertices[i*4+1]={{x+nx,y+ny},ink,{}};
            vertices[i*4+2]={{x-nx,y-ny},ink,{}};vertices[i*4+3]={{x-nx-ox,y-ny-oy},clear,{}};
            if(i<steps)for(int band=0;band<3;++band){int k=i*18+band*6,v=i*4+band;indices[k]=v;indices[k+1]=v+1;indices[k+2]=v+4;indices[k+3]=v+1;indices[k+4]=v+5;indices[k+5]=v+4;}
        }
        SDL_RenderGeometry(p.r,nullptr,vertices.data(),int(vertices.size()),indices.data(),int(indices.size()));
    }
}
inline void menuBackground(Painter &p){
    auto a=p.theme.accent();
    p.gradient({0,0,1100,height},{10,15,18,255},{4,7,10,255});
    p.glow({435,60,690,560},opacity(a,33));
    curve(p,{-90,105},{300,30},{540,100},{825,-45},opacity(a,145),2.2f);
    curve(p,{160,680},{620,470},{780,610},{1200,488},opacity(a,135),2.5f);
    curve(p,{-120,156},{210,52},{550,108},{720,-35},opacity(a,30),1.2f);
    curve(p,{360,59},{520,41},{630,27},{725,-3},opacity(a,230),2);
    curve(p,{447,613},{610,552},{785,569},{905,546},opacity(a,180),2);
    p.line(40,578,1060,578,{48,59,63,200},1);
}
inline void glyph(Painter &p,int type,float x,float y,SDL_Color color){
    if(type==0){p.icon(Icon::Play,x,y,32,color);return;}
    if(type==1){
        constexpr double pi=3.141592653589793;
        for(int i=0;i<32;++i){
            auto point=[&](int j){double angle=j*2*pi/32;float radius=(j%4==0||j%4==3)?15:11.5f;return SDL_FPoint{x+16+float(std::cos(angle))*radius,y+16+float(std::sin(angle))*radius};};
            auto a=point(i),b=point((i+1)%32);p.line(a.x,a.y,b.x,b.y,color,2.1f);
        }
        for(int i=0;i<20;++i){double t=i*2*pi/20,n=(i+1)*2*pi/20;p.line(x+16+float(std::cos(t))*5,y+16+float(std::sin(t))*5,x+16+float(std::cos(n))*5,y+16+float(std::sin(n))*5,color,2);}
    }else{
        for(int i=0;i<3;++i){float yy=y+5+i*11,xx=x+(i==1?10:23);p.line(x,yy,x+32,yy,color,2.5f);p.round({int(xx)-4,int(yy)-4,9,9},4,color);}
    }
}
inline void navigationButton(Painter &p,int index,const std::string &label,bool focused){
    auto b=navigation[size_t(index)];auto a=p.theme.accent();
    float focus=p.animate(900+index,focused),lift=focus*2;
    bool primary=index==0;
    if(primary||focus>.01f)p.glow({float(b.x)-50,float(b.y)-42-lift,float(b.w)+100,float(b.h)+84},opacity(a,Uint8((primary?70:0)+focus*70)));
    auto fill=Painter::mix({15,22,26,255},a,primary?.075f+focus*.06f:focus*.05f);
    if(primary){p.round(b,18,Painter::mix(a,white,focus*.12f),-lift);p.round({b.x+3,b.y+3,b.w-6,b.h-6},15,fill,-lift);}
    else p.card(b,fill,Painter::mix({53,65,69,255},a,focus*.85f),18,-lift);
    if(primary){p.round({b.x+3,b.y+3,b.w-6,3},1,opacity(a,Uint8(130+focus*100)),-lift);}
    p.line(b.x+1,b.y+23-lift,b.x+1,b.y+b.h-23-lift,opacity(a,primary?255:150),2.5f);
    p.line(b.x+b.w-36,b.y+6-lift,b.x+b.w-14,b.y+27-lift,opacity(a,Uint8(50+focus*100)),2);
    glyph(p,index,float(b.x+28),b.y+(b.h-32)/2.0f-lift,primary?a:white);
    p.text(label,b.x+91,b.y+(primary?29:23)-lift,white,primary?p.title:p.heading);
    p.icon(Icon::Chevron,b.x+b.w-36,b.y+(b.h-20)/2.0f-lift,20,opacity(a,Uint8(80+focus*175)));
}
inline void menuHeader(Painter &p,Screen screen,bool backHovered,ConsoleColor theme){
    auto a=p.theme.accent();
    if(screen==Screen::Main){
        p.text("XR1-L Player",42,32,white,p.heading);p.text("1.5.14",226,39,muted,p.small);
        p.round({847,37,13,13},4,a);p.text(themeName(theme),874,33,white,p.normal);
        p.text("MAIN MENU",48,138,muted,p.tiny);
    }else{
        p.text(screen==Screen::Settings?"SETTINGS":"PREFERENCES",40,34,white,p.heading);
        p.text("XR1-L Player",40,72,muted,p.small);
        p.button(back,"Back  /  Esc",backHovered,false,p.small);
    }
}
inline bool logoPanel(Painter &p,double elapsed,bool reducedMotion,SDL_Texture *video=nullptr){
    auto a=p.theme.accent();
    p.glow({425,124,690,430},opacity(a,25));
    // Interrupted rails follow the device proportions rather than enclosing a
    // square image tile. Alpha artwork has no visible rectangular image edge.
    const std::array<SDL_FPoint,11> rail={SDL_FPoint{452,244},{462,209},{486,185},{637,185},{654,174},{1026,174},{1058,206},{1058,466},{1032,506},{866,506},{850,518}};
    for(size_t i=1;i<rail.size();++i){p.line(rail[i-1].x,rail[i-1].y,rail[i].x,rail[i].y,opacity(a,16),8);p.line(rail[i-1].x,rail[i-1].y,rail[i].x,rail[i].y,opacity(a,105),1.4f);}
    p.line(452,268,452,473,opacity(a,170),2);
    p.line(475,515,817,515,opacity(a,110),1.4f);
    p.line(670,174,752,174,a,3);p.line(903,506,1028,506,a,3);
    bool videoShown=false;
    if(video){
        const auto &preview=mainPreview;
        p.card({preview.x-4,preview.y-4,preview.w+8,preview.h+8},surface,border,6);
        videoShown=SDL_RenderCopy(p.r,video,nullptr,&preview)==0;
    }else{
        // The backing follows the device silhouette inside its opaque border, not
        // the square image bounds. It supplies the original accent-colored holes
        // and lettering while preserving the original dark ink and unchanged mask.
        float bob=deviceBob(elapsed,reducedMotion);double tilt=deviceTilt(elapsed,reducedMotion);
        Box face{int(logoX+logoSize*.067f),int(logoY+logoSize*.235f),int(logoSize*.87f),int(logoSize*.535f)};
        face.x+=5;face.y+=5;face.w-=10;face.h-=10;
        SDL_FPoint center{logoX+logoSize*.5f-face.x,logoY+logoSize*.5f-face.y};
        p.round(face,logoSize*.075f,a,bob,tilt,&center);
        p.icon(Icon::Capture,logoX,logoY+bob,logoSize,p.theme.ink(),false,tilt);
    }
    p.text("XR1 LITE",480,545,muted,p.tiny);
    p.text("1080p / 60 fps     HDMI stereo audio",748,545,muted,p.tiny);
    return videoShown;
}
inline void themeCard(Painter &p,int index,bool selected,bool focused){
    auto b=themes[size_t(index)];MenuTheme option;option.choice=static_cast<ConsoleColor>(index);auto swatch=option.accent();
    p.applyTheme=false;
    float f=p.animate(920+index,focused);
    p.card(b,Painter::mix({17,24,29,255},swatch,selected?.08f:f*.04f),(selected||focused)?swatch:border,16,-f*2);
    p.round({b.x+24,b.y+25,36,36},11,swatch,-f*2);
    if(selected)p.icon(Icon::Check,b.x+32,b.y+33-f*2,20,option.ink());
    p.text(themeName(option.choice),b.x+24,b.y+82-f*2,white,p.heading);
    p.text(selected?"SELECTED":"Select color",b.x+24,b.y+119-f*2,selected?swatch:muted,p.tiny);
    p.applyTheme=true;
}
}
