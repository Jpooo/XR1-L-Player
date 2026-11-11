#include "../src/ui.hpp"
#include <cassert>
#include <iostream>

int main(){
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS,"permonitorv2");SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING,"1");
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    auto *window=SDL_CreateWindow("Native icon resources",0,0,600,400,SDL_WINDOW_HIDDEN|SDL_WINDOW_ALLOW_HIGHDPI);assert(window);
    SDL_SysWMinfo info{};SDL_VERSION(&info.version);assert(SDL_GetWindowWMInfo(window,&info));
    {
        ui::NativeWindowIcons icons;
        for(int repeat=0;repeat<4;++repeat)for(int theme=0;theme<3;++theme){
            auto color=static_cast<ui::ConsoleColor>(theme);assert(icons.set(window,color));
            ui::MenuTheme palette;palette.choice=color;auto expected=palette.accent();
            int lastWidth=0;
            for(int type:{ICON_SMALL,ICON_BIG}){
                auto icon=reinterpret_cast<HICON>(SendMessageW(info.info.win.window,WM_GETICON,type,0));assert(icon);
                ICONINFO image{};assert(GetIconInfo(icon,&image));BITMAP bitmap{};assert(GetObjectW(image.hbmColor,sizeof(bitmap),&bitmap));
                assert(bitmap.bmWidth>=16&&bitmap.bmHeight==bitmap.bmWidth);
                if(type==ICON_BIG)assert(bitmap.bmWidth>lastWidth);
                lastWidth=bitmap.bmWidth;
                BITMAPINFO format{};format.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);format.bmiHeader.biWidth=bitmap.bmWidth;format.bmiHeader.biHeight=-bitmap.bmHeight;format.bmiHeader.biPlanes=1;format.bmiHeader.biBitCount=32;format.bmiHeader.biCompression=BI_RGB;
                std::vector<Uint32> pixels(size_t(bitmap.bmWidth)*bitmap.bmHeight);auto dc=GetDC(nullptr);
                assert(GetDIBits(dc,image.hbmColor,0,bitmap.bmHeight,pixels.data(),&format,DIB_RGB_COLORS));ReleaseDC(nullptr,dc);
                assert(std::any_of(pixels.begin(),pixels.end(),[&](Uint32 p){return ((p>>16)&255)==expected.r&&((p>>8)&255)==expected.g&&(p&255)==expected.b;}));
                DeleteObject(image.hbmColor);DeleteObject(image.hbmMask);
            }
        }
    }
    assert(!SendMessageW(info.info.win.window,WM_GETICON,ICON_SMALL,0));assert(!SendMessageW(info.info.win.window,WM_GETICON,ICON_BIG,0));
    SDL_DestroyWindow(window);SDL_Quit();std::cout<<"PASS themed native small/large icon resources, theme refresh and handle cleanup\n";
}
