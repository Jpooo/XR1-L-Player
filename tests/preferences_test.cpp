// Exercise preference persistence using the same entry points as the UI.
#define main xr1_application_main
#include "../src/main.cpp"
#undef main
#include <cassert>
int main(){
#ifdef _WIN32
    auto id=GetCurrentProcessId();
#else
    auto id=getpid();
#endif
    auto folder=std::filesystem::temp_directory_path()/("xr1-color-preference-"+std::to_string(id)+"-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
#ifdef _WIN32
    _putenv_s("XDG_CONFIG_HOME",folder.string().c_str());
#else
    setenv("XDG_CONFIG_HOME",folder.string().c_str(),1);
#endif
    auto path=settingsPath(true);std::filesystem::create_directories(path.parent_path());
    {
        std::ofstream out(path);
        out<<"video \"Capture Card #1\"\naudio \"USB HDMI Audio\"\noutput \"Desktop speakers\"\nformat \"nv12\"\ncustom \"keep this field\"\ntheme \"xbox\"\n";
    }
    assert(!Options{}.fsr&&Options{}.sharpening==0);
    Options legacy;loadSettings(legacy);assert(legacy.video=="Capture Card #1");
    assert(saveColorPreference(ui::ConsoleColor::PlayStation));
    assert(std::filesystem::exists(path));path=settingsPath();
    Options blue;loadSettings(blue);
    assert(blue.video=="Capture Card #1"&&blue.audio=="USB HDMI Audio"&&blue.output=="Desktop speakers"&&blue.format=="nv12");
    assert(blue.colorTheme==ui::ConsoleColor::PlayStation);
    {std::ifstream in(path);std::string data((std::istreambuf_iterator<char>(in)),{});assert(data.find("custom \"keep this field\"")!=std::string::npos);}
    assert(updateSettings({{"upscaling","fsr1"}}));Options fsr;loadSettings(fsr);assert(fsr.fsr);
    assert(updateSettings({{"sharpening","high"}}));Options sharpened;loadSettings(sharpened);assert(sharpened.fsr&&sharpened.sharpening==3);
    assert(saveMotionPreference(true));Options motion;loadSettings(motion);assert(motion.reduceMotion&&motion.fsr&&motion.sharpening==3);
    assert(saveVolumePreference(.42f,true));
    Options volume;loadSettings(volume);assert(std::abs(volume.volume-.42f)<.0001f&&volume.muted);
    assert(saveSettings(blue.video,blue.audio,blue.output,blue.format,blue.colorTheme,volume.volume,volume.muted));
    Options reopened;loadSettings(reopened);assert(reopened.colorTheme==ui::ConsoleColor::PlayStation&&reopened.video==blue.video&&reopened.reduceMotion);
    assert(saveColorPreference(ui::ConsoleColor::Nintendo));
    Options red;loadSettings(red);assert(red.colorTheme==ui::ConsoleColor::Nintendo&&red.audio==blue.audio);
    assert(saveColorPreference(ui::ConsoleColor::Xbox));
    Options green;loadSettings(green);assert(green.colorTheme==ui::ConsoleColor::Xbox&&green.output==blue.output&&green.muted&&std::abs(green.volume-.42f)<.0001f);
    assert(saveVolumePreference(0,false));Options zero;loadSettings(zero);assert(zero.volume==0&&!zero.muted);
    assert(saveVolumePreference(1,true));Options full;loadSettings(full);assert(full.volume==1&&full.muted);
    {std::ifstream in(path);std::string data((std::istreambuf_iterator<char>(in)),{});assert(data.find("custom \"keep this field\"")!=std::string::npos);}
    for(const char *bad:{"4","-1","nan","invalid"}){assert(updateSettings({{"sharpening",bad}}));Options invalid;loadSettings(invalid);assert(invalid.sharpening==0);}
    for(const char *bad:{"nan","inf","-0.5","1.5","0.5junk","invalid"}){assert(updateSettings({{"volume",bad}}));Options invalid;loadSettings(invalid);assert(invalid.volume==.7f);}
    assert(updateSettings({{"window_width","1600"},{"window_height","900"},{"fullscreen","true"},{"format","mjpeg"}}));
    Options window;loadSettings(window);assert(window.windowWidth==1600&&window.windowHeight==900&&window.fullscreen&&window.format=="auto");
    assert(saveVolumePreference(.5f,false));Options retainedWindow;loadSettings(retainedWindow);assert(retainedWindow.windowWidth==1600&&retainedWindow.fullscreen);
    for(const char *bad:{"-1","0","999999","1100junk","invalid"}){assert(updateSettings({{"window_width",bad},{"window_height",bad}}));Options invalid;loadSettings(invalid);assert(invalid.windowWidth==1100&&invalid.windowHeight==650);}
    for(const char *bad:{"uyvy","i420","yv12","mjpeg"}){char arg0[]="app",arg1[]="--format";char *argv[]={arg0,arg1,const_cast<char*>(bad)};bool rejected=false;try{parse(3,argv);}catch(const std::runtime_error&){rejected=true;}assert(rejected);}
    char arg0[]="app",arg1[]="--windowed";char *argv[]={arg0,arg1};assert(!parse(2,argv).fullscreen);
    auto temporary=path;
#ifdef _WIN32
    temporary+=".tmp."+std::to_string(GetCurrentProcessId());
#else
    temporary+=".tmp."+std::to_string(getpid());
#endif
    std::ifstream before(path);std::string original((std::istreambuf_iterator<char>(before)),{});before.close();
    std::filesystem::create_directory(temporary);assert(!saveVolumePreference(.1f,false));
    std::ifstream after(path);std::string retained((std::istreambuf_iterator<char>(after)),{});assert(original==retained);after.close();
    std::filesystem::remove_all(folder);
    std::cout<<"PASS saved colors, volume/mute, migration, device/unknown-field preservation, numeric validation and failed-write preservation\n";
}
