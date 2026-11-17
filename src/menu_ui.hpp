#pragma once
#include "ui.hpp"

namespace ui {
inline constexpr SDL_Color studioBlack{0,0,0,255},studioPanel{8,8,8,255},studioField{13,13,13,255};
inline constexpr SDL_Color studioBorder{36,36,36,255},studioMuted{153,157,162,255};

inline const char *themeName(ConsoleColor choice) {
    constexpr const char *names[]={"Neon Green","Electric Blue","Crimson Red"};
    return names[static_cast<size_t>(choice)];
}

inline SDL_Color studioAccent(ConsoleColor choice) {
    constexpr SDL_Color accents[]={{155,239,80,255},{112,170,255,255},{241,106,119,255}};
    return accents[static_cast<size_t>(choice)];
}

inline SDL_Color opacity(SDL_Color color,Uint8 alpha) {
    color.a=alpha;
    return color;
}

inline void menuBackground(Painter &p) {
    p.rect(0,0,1100,height,studioBlack);
    p.rect(0,70,1100,1,studioBorder);
    p.rect(sidebarWidth,71,1,height-71,studioBorder);
    p.rect(workspaceLeft,620,800,1,studioBorder);
}

inline void studioButton(Painter &p,Box box,const std::string &label,bool focused,bool primary=false) {
    const auto accent=studioAccent(p.theme.choice);
    const SDL_Color fill=primary?Painter::mix(studioField,accent,focused?.21f:.11f):
        focused?SDL_Color{24,24,24,255}:studioField;
    p.card(box,fill,focused?accent:studioBorder,8);
    int width=0,height=0;
    TTF_SizeUTF8(p.small,label.c_str(),&width,&height);
    p.text(label,box.x+(box.w-width/p.fontScale)/2,box.y+(box.h-height/p.fontScale)/2,white,p.small);
}

inline void menuHeader(Painter &p,Screen screen,bool backFocused) {
    const auto accent=studioAccent(p.theme.choice);
    p.card({34,18,35,35},studioPanel,Painter::mix(studioBorder,accent,.32f),7);
    p.bitmap(std::string("branding/xr1-play-")+colorKey(p.theme.choice)+".bmp",{36,20,31,31},{255,255,255,255});
    p.text("XR1-L Player",81,15,white);
    p.text("CAPTURE STUDIO",82,39,studioMuted,p.tiny);
    if(screen!=Screen::Main)studioButton(p,back,"Main menu",backFocused);
}

inline void navigationButton(Painter &p,int index,const char *label,bool selected,bool focused) {
    constexpr std::array<const char*,4> descriptions={"Open the HDMI feed","Video and audio devices","Appearance and motion","Close the application"};
    const auto box=navigation[size_t(index)];
    const auto accent=studioAccent(p.theme.choice);
    const float hover=p.animate(550+index,focused);
    const auto fill=selected?Painter::mix(studioPanel,accent,.065f):
        Painter::mix(studioBlack,studioField,hover);
    const auto edge=focused?accent:selected?Painter::mix(studioBorder,accent,.27f):fill;
    p.card(box,fill,edge,10);
    if(selected)p.rect(box.x+1,box.y+9,2,box.h-18,accent);
    p.card({box.x+10,box.y+11,29,29},studioField,selected?edge:studioBorder,7);
    const float x=float(box.x+17),y=float(box.y+18);
    const auto icon=selected?accent:studioMuted;
    if(index==0)p.icon(Icon::Play,x,y,15,icon);
    if(index==1){
        p.line(x,y+4,x+15,y+4,icon,1.4f);p.line(x,y+11,x+15,y+11,icon,1.4f);
        p.round({int(x)+3,int(y)+2,4,4},2,icon);p.round({int(x)+9,int(y)+9,4,4},2,icon);
    }
    if(index==2){
        for(int row=0;row<2;++row)for(int col=0;col<2;++col)
            p.card({int(x)+col*8,int(y)+row*8,7,7},studioField,icon,2);
    }
    if(index==3){
        p.line(x+6,y,x,y,icon,1.4f);p.line(x,y,x,y+15,icon,1.4f);
        p.line(x,y+15,x+6,y+15,icon,1.4f);
        p.line(x+5,y+7.5f,x+15,y+7.5f,icon,1.4f);
        p.line(x+11,y+3.5f,x+15,y+7.5f,icon,1.4f);
        p.line(x+11,y+11.5f,x+15,y+7.5f,icon,1.4f);
    }
    p.text(label,box.x+48,box.y+7,white,p.small);
    p.fit(descriptions[size_t(index)],box.x+48,box.y+27,box.w-54,studioMuted,p.tiny);
}

inline void studioSidebar(Painter &p,Screen screen,const char *playLabel,int focused) {
    p.text("Capture",33,112,white,p.heading);
    p.text("studio",33,144,white,p.heading);
    p.text("HDMI video and audio",34,183,studioMuted,p.small);
    p.text("WORKSPACE",34,225,studioMuted,p.tiny);
    const std::array<const char*,4> labels={playLabel,"Settings","Preferences","Exit"};
    for(size_t i=0;i<labels.size();++i)
        navigationButton(p,int(i),labels[i],int(i)==static_cast<int>(screen),int(i)==focused);
    p.round({34,630,6,6},3,studioAccent(p.theme.choice));
    p.text("XR1-L PLAYER 1.0",47,625,studioMuted,p.tiny);
}

inline bool logoPanel(Painter &p,double elapsed,bool reducedMotion,SDL_Texture *video=nullptr) {
    const auto accent=studioAccent(p.theme.choice);
    p.card(stage,studioBlack,Painter::mix(studioBorder,accent,.16f),16);
    int width=0,height=0;
    if(video&&SDL_QueryTexture(video,nullptr,nullptr,&width,&height)==0&&width>0&&height>0){
        // Fit the entire texture inside the Studio frame, with black space on
        // the unused axis. Draw it directly so the preview shares the live feed.
        const float scale=std::min(float(previewArea.w)/width,float(previewArea.h)/height);
        const float fittedWidth=width*scale,fittedHeight=height*scale;
        const SDL_FRect destination{previewArea.x+(previewArea.w-fittedWidth)/2,
                                    previewArea.y+(previewArea.h-fittedHeight)/2,
                                    fittedWidth,fittedHeight};
        if(SDL_RenderCopyF(p.r,video,nullptr,&destination)==0)return true;
    }

    p.glow({405,158,510,320},opacity(accent,21));
    constexpr float artworkHeight=logoSize*2/3;
    constexpr float lightWidth=logoSize*4/3,lightHeight=logoSize;
    const float bob=reducedMotion?0.0f:float(-4*(1-std::cos(elapsed*6.283185307/7)));
    const double angle=-4+deviceTilt(elapsed,reducedMotion);
    p.glow({logoX-21,logoY+54+bob,logoSize+42,artworkHeight-45},opacity(accent,83));
    p.bitmap("studio/device-light.bmp",
        {logoX-(lightWidth-logoSize)/2,logoY-(lightHeight-artworkHeight)/2+bob,lightWidth,lightHeight},accent,angle);
    p.bitmap("studio/device.bmp",{logoX,logoY+bob,logoSize,artworkHeight},{255,255,255,255},angle);
    p.text("XR1 LITE",stage.x+19,stage.y+18,studioMuted,p.tiny);
    p.text("PREVIEW AREA",stage.x+692,stage.y+18,studioMuted,p.tiny);
    return false;
}

inline void overviewCard(Painter &p,int index,const char *label,const std::string &value,bool focused=false) {
    const auto box=overview[size_t(index)];
    const auto accent=studioAccent(p.theme.choice);
    p.card(box,focused?studioField:studioPanel,focused?accent:studioBorder,12);
    constexpr Icon icons[]={Icon::Video,Icon::Speaker,Icon::Chevron};
    p.icon(icons[index],box.x+14,box.y+15,15,accent);
    p.text(label,box.x+36,box.y+15,studioMuted,p.tiny);
    p.fit(value,box.x+14,box.y+41,box.w-30,white,p.normal);
}

inline void themeCard(Painter &p,int index,bool selected,bool focused) {
    const auto box=themes[size_t(index)];
    const auto choice=static_cast<ConsoleColor>(index);
    const auto swatch=studioAccent(choice);
    p.card(box,studioField,focused||selected?swatch:studioBorder,11);
    p.round({box.x+14,box.y+22,24,24},12,swatch);
    if(selected)p.icon(Icon::Check,box.x+19,box.y+27,14,studioBlack);
    p.text(themeName(choice),box.x+48,box.y+24,white,p.normal);
}
}
