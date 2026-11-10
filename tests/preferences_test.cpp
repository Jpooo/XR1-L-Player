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
    auto folder=std::filesystem::temp_directory_path()/("xr1-color-preference-"+std::to_string(id));
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
    Options legacy;loadSettings(legacy);assert(legacy.video=="Capture Card #1");
    assert(saveColorPreference(ui::ConsoleColor::PlayStation));
    assert(std::filesystem::exists(path));path=settingsPath();
    Options blue;loadSettings(blue);
    assert(blue.video=="Capture Card #1"&&blue.audio=="USB HDMI Audio"&&blue.output=="Desktop speakers"&&blue.format=="nv12");
    assert(blue.colorTheme==ui::ConsoleColor::PlayStation);
    {std::ifstream in(path);std::string data((std::istreambuf_iterator<char>(in)),{});assert(data.find("custom \"keep this field\"")!=std::string::npos);}
    saveSettings(blue.video,blue.audio,blue.output,blue.format,blue.colorTheme);
    Options reopened;loadSettings(reopened);assert(reopened.colorTheme==ui::ConsoleColor::PlayStation&&reopened.video==blue.video);
    assert(saveColorPreference(ui::ConsoleColor::Nintendo));
    Options red;loadSettings(red);assert(red.colorTheme==ui::ConsoleColor::Nintendo&&red.audio==blue.audio);
    assert(saveColorPreference(ui::ConsoleColor::Xbox));
    Options green;loadSettings(green);assert(green.colorTheme==ui::ConsoleColor::Xbox&&green.output==blue.output);
    std::filesystem::remove_all(folder);
    std::cout<<"PASS all three saved color preferences, device/format preservation, unknown-field preservation and reload after device settings save\n";
}
