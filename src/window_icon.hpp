#pragma once
#include "menu_theme.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <SDL_syswm.h>
#endif

namespace ui {
// Use the matching multi-resolution executable resource for the Windows title
// bar and task switcher. Own each size separately: LR_SHARED may return a
// previously cached icon at the wrong size after a monitor DPI change.
class NativeWindowIcons {
#ifdef _WIN32
    HWND owner=nullptr;HICON small=nullptr,large=nullptr;UINT lastDpi=0;int lastResource=0;
#endif
public:
    NativeWindowIcons()=default;
    NativeWindowIcons(const NativeWindowIcons&)=delete;
    NativeWindowIcons &operator=(const NativeWindowIcons&)=delete;
    ~NativeWindowIcons(){
#ifdef _WIN32
        if(owner&&IsWindow(owner)){
            if(reinterpret_cast<HICON>(SendMessageW(owner,WM_GETICON,ICON_SMALL,0))==small)SendMessageW(owner,WM_SETICON,ICON_SMALL,0);
            if(reinterpret_cast<HICON>(SendMessageW(owner,WM_GETICON,ICON_BIG,0))==large)SendMessageW(owner,WM_SETICON,ICON_BIG,0);
        }
        if(small)DestroyIcon(small);
        if(large)DestroyIcon(large);
#endif
    }
    bool set(SDL_Window *window,ConsoleColor color){
#ifdef _WIN32
    SDL_SysWMinfo info{};SDL_VERSION(&info.version);
    if(!SDL_GetWindowWMInfo(window,&info)||info.subsystem!=SDL_SYSWM_WINDOWS)return false;
    auto module=GetModuleHandleW(nullptr);int resource=1+static_cast<int>(color);
    if(!FindResourceW(module,MAKEINTRESOURCEW(resource),MAKEINTRESOURCEW(14)))return false;
    auto user=GetModuleHandleW(L"user32.dll");
    using WindowDpi=UINT(WINAPI*)(HWND);using MetricDpi=int(WINAPI*)(int,UINT);
    WindowDpi windowDpi=nullptr;MetricDpi metricDpi=nullptr;
    auto windowProc=GetProcAddress(user,"GetDpiForWindow"),metricProc=GetProcAddress(user,"GetSystemMetricsForDpi");
    static_assert(sizeof(windowDpi)==sizeof(windowProc)&&sizeof(metricDpi)==sizeof(metricProc));
    std::memcpy(&windowDpi,&windowProc,sizeof(windowDpi));std::memcpy(&metricDpi,&metricProc,sizeof(metricDpi));
    UINT dpi=windowDpi?windowDpi(info.info.win.window):96;if(!dpi)dpi=96;
    if(owner==info.info.win.window&&lastDpi==dpi&&lastResource==resource)return true;
    auto metric=[&](int index){return metricDpi?metricDpi(index,dpi):GetSystemMetrics(index);};
    auto nextSmall=static_cast<HICON>(LoadImageW(module,MAKEINTRESOURCEW(resource),IMAGE_ICON,metric(SM_CXSMICON),metric(SM_CYSMICON),0));
    auto nextLarge=static_cast<HICON>(LoadImageW(module,MAKEINTRESOURCEW(resource),IMAGE_ICON,metric(SM_CXICON),metric(SM_CYICON),0));
    if(!nextSmall||!nextLarge){if(nextSmall)DestroyIcon(nextSmall);if(nextLarge)DestroyIcon(nextLarge);return false;}
    SendMessageW(info.info.win.window,WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(nextSmall));
    SendMessageW(info.info.win.window,WM_SETICON,ICON_BIG,reinterpret_cast<LPARAM>(nextLarge));
    if(small)DestroyIcon(small);
    if(large)DestroyIcon(large);
    small=nextSmall;large=nextLarge;owner=info.info.win.window;lastDpi=dpi;lastResource=resource;
    return true;
#else
    (void)window;(void)color;return false;
#endif
}
};
}
