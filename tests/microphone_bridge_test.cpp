#include "../src/microphone_bridge.hpp"
#include <cassert>
#include <iostream>
static std::string hex(const std::string &s){std::ostringstream out;for(unsigned char c:s)out<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c);return out.str();}
static double seconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int main(int argc,char **argv){
    if(argc>2&&std::string(argv[1])=="--bridge-dir"){
        std::filesystem::path dir=argv[2];std::string state="ready",message="USB microphone";bool frozen=false;
        for(int i=0;i<500;++i){
            if(!std::filesystem::exists(dir))return 0;
            for(const auto &entry:std::filesystem::directory_iterator(dir)){
                auto name=entry.path().filename().string();if(name.rfind("cmd-",0)||name.find(".tmp")!=std::string::npos)continue;
                std::ifstream input(entry.path());std::string command;std::getline(input,command);input.close();std::filesystem::remove(entry.path());
                if(command=="quit")return 0;
                if(command=="mute\t1")state="muted";
                if(command=="crash")return 3;
                if(command=="hang")frozen=true;
            }
            if(frozen){SDL_Delay(10);continue;}
            auto tmp=dir/"status.tmp";{std::ofstream out(tmp);out<<"state="<<hex(state)<<"\nmessage="<<hex(message)<<"\nupdated="<<hex(std::to_string(i))<<"\n";}
            std::error_code ec;std::filesystem::rename(tmp,dir/"status",ec);SDL_Delay(10);
        }
        return 4;
    }
    assert(SDL_Init(SDL_INIT_TIMER)==0);
    auto source=std::filesystem::absolute(argv[0]);
    auto copy=std::filesystem::temp_directory_path()/("xr1 microphone test "+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(copy);
    auto executable=copy/source.filename();std::filesystem::copy_file(source,executable);
    SDL_setenv("XR1_MIC_BRIDGE_EXECUTABLE",executable.string().c_str(),1);
    MicrophoneBridge bridge;assert(!bridge.active());assert(bridge.start());
    auto until=[&](auto pred){double deadline=seconds()+5;while(seconds()<deadline){bridge.poll(seconds());if(pred())return true;SDL_Delay(20);}return false;};
    assert(until([&]{return bridge.state()=="ready";}));
    assert(bridge.get("message")=="USB microphone");
    assert(bridge.command("mute\t1"));assert(until([&]{return bridge.state()=="muted";}));assert(bridge.active());
    assert(bridge.command("crash"));assert(until([&]{return bridge.state()=="error";}));assert(!bridge.active());
    assert(bridge.start());assert(until([&]{return bridge.state()=="ready";}));
    assert(bridge.command("hang"));SDL_Delay(100);bridge.poll(seconds());SDL_Delay(50);bridge.poll(seconds()+11);
    assert(bridge.state()=="error"&&!bridge.active()&&!bridge.running());
    assert(bridge.start());assert(until([&]{return bridge.state()=="ready";}));
    double began=seconds();bridge.shutdown();assert(seconds()-began<1);assert(!bridge.running());
    std::filesystem::remove_all(copy);SDL_Quit();
    std::cout<<"PASS microphone process: paths with spaces, commands/status, failure recovery and bounded shutdown\n";
}
