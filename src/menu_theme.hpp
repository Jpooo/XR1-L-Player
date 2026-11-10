#pragma once
#include "core.hpp"
namespace ui {
enum class ConsoleColor {Xbox,PlayStation,Nintendo};
inline const char *colorKey(ConsoleColor choice){return choice==ConsoleColor::PlayStation?"playstation":choice==ConsoleColor::Nintendo?"nintendo":"xbox";}
inline ConsoleColor parseColor(const std::string &key){return key=="playstation"?ConsoleColor::PlayStation:key=="nintendo"?ConsoleColor::Nintendo:ConsoleColor::Xbox;}
struct MenuTheme {
    ConsoleColor choice=ConsoleColor::Xbox;
    SDL_Color accent()const{return choice==ConsoleColor::PlayStation?SDL_Color{79,143,247,255}:choice==ConsoleColor::Nintendo?SDL_Color{244,67,78,255}:SDL_Color{155,240,11,255};}
    SDL_Color ink()const{return choice==ConsoleColor::PlayStation?SDL_Color{9,18,34,255}:choice==ConsoleColor::Nintendo?SDL_Color{29,10,14,255}:SDL_Color{14,24,5,255};}
    // Bridge the existing dashboard's green accent/tints to the selected
    // palette. This is used only for UI primitives, never the video texture.
    SDL_Color resolve(SDL_Color color)const{
        if(choice==ConsoleColor::Xbox)return color;
        Uint8 alpha=color.a;
        if(color.r==155&&color.g==240&&color.b==11)color=accent();
        else if(color.g<=80&&color.g>color.r&&color.g>color.b){
            if(color.r<=16&&color.b<=7&&color.g<=34)color=ink();
            else {auto a=accent();float gray=(color.r+color.g+color.b)/3.0f;constexpr float tint=.13f;color={Uint8(gray*(1-tint)+a.r*tint),Uint8(gray*(1-tint)+a.g*tint),Uint8(gray*(1-tint)+a.b*tint),alpha};}
        }
        color.a=alpha;return color;
    }
};
}
