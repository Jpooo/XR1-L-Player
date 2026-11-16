#pragma once
#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
extern char **environ;
#endif

// Optional, isolated Remote Play transport. Native capture/audio never depend
// on its network I/O; the UI exchanges only small atomic local status files.
class MicrophoneBridge {
    std::filesystem::path directory;
    std::map<std::string,std::string> fields;
    unsigned sequence=0;
    double lastPoll=0,lastHeartbeat=0;
    std::string heartbeat;
#ifdef _WIN32
    HANDLE process=nullptr;
#else
    pid_t process=-1;
#endif
    static std::string decode(const std::string &value){
        if(value.size()%2||value.size()>4096)return {};
        std::string result;result.reserve(value.size()/2);
        auto digit=[](char c)->int{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
        for(size_t i=0;i<value.size();i+=2){int a=digit(value[i]),b=digit(value[i+1]);if(a<0||b<0)return {};char c=char(a*16+b);if(c=='\0'||c=='\r'||c=='\n')c=' ';result+=c;}
        return result;
    }
    bool alive(){
#ifdef _WIN32
        if(!process)return false;
        if(WaitForSingleObject(process,0)==WAIT_TIMEOUT)return true;
        CloseHandle(process);process=nullptr;return false;
#else
        if(process<0)return false;
        if(waitpid(process,nullptr,WNOHANG)==0)return true;
        process=-1;return false;
#endif
    }
public:
    MicrophoneBridge()=default;
    MicrophoneBridge(const MicrophoneBridge&)=delete;
    MicrophoneBridge& operator=(const MicrophoneBridge&)=delete;
    ~MicrophoneBridge(){shutdown();}
    std::string get(const std::string &key,const std::string &fallback="")const{auto it=fields.find(key);return it==fields.end()?fallback:it->second;}
    int count(const std::string &key)const{try{return std::clamp(std::stoi(get(key,"0")),0,64);}catch(...){return 0;}}
    std::string state()const{return get("state","off");}
    bool active()const{auto s=state();return s=="connecting"||s=="waiting"||s=="transmitting"||s=="muted";}
    bool running(){return alive();}
    void poll(double now){
        if(now-lastPoll<.2)return;
        lastPoll=now;
        if(directory.empty())return;
        if(!alive()){fields["state"]="error";fields["message"]="Microphone helper stopped. Reopen this page to retry.";return;}
        if(!lastHeartbeat)lastHeartbeat=now;
        std::ifstream input(directory/"status");std::string line;std::map<std::string,std::string> next;
        for(int i=0;i<150&&std::getline(input,line);++i){if(line.size()>8192)break;auto eq=line.find('=');if(eq!=std::string::npos)next[line.substr(0,eq)]=decode(line.substr(eq+1));}
        if(next.count("state")&&next.count("updated")){
            if(next["updated"]!=heartbeat){heartbeat=next["updated"];lastHeartbeat=now;}
            fields=std::move(next);
        }
        if(lastHeartbeat&&now-lastHeartbeat>10){shutdown();fields["state"]="error";fields["message"]="Microphone helper stopped responding and was closed. Reopen this page to retry.";}
    }
    bool start(){
        if(alive()&&state()!="error")return true;
        shutdown();
        char *base=SDL_GetBasePath();std::filesystem::path root=base?base:".";SDL_free(base);
#ifdef _WIN32
        auto executable=root/"microphone-bridge/XR1-Microphone.exe";
        unsigned long owner=GetCurrentProcessId();
#else
        auto executable=root/"microphone-bridge/xr1-microphone";
        unsigned long owner=static_cast<unsigned long>(getpid());
#endif
        if(const char *overridePath=SDL_getenv("XR1_MIC_BRIDGE_EXECUTABLE"))executable=overridePath;
        if(!std::filesystem::is_regular_file(executable)){fields={{"state","error"},{"message","This package does not include the experimental microphone helper."}};return false;}
        std::error_code ec;
        auto nonce=std::chrono::steady_clock::now().time_since_epoch().count();
        directory=std::filesystem::temp_directory_path(ec)/("xr1-mic-"+std::to_string(owner)+"-"+std::to_string(nonce));
        if(ec||!std::filesystem::create_directory(directory,ec)){directory.clear();fields={{"state","error"},{"message","Cannot create the microphone session folder."}};return false;}
        std::filesystem::permissions(directory,std::filesystem::perms::owner_all,std::filesystem::perm_options::replace,ec);
        std::string devApp=SDL_getenv("XR1_MIC_BRIDGE_APP")?SDL_getenv("XR1_MIC_BRIDGE_APP"):"";
#ifdef _WIN32
        auto quote=[](const std::wstring &s){std::wstring out=L"\"";unsigned slashes=0;for(wchar_t c:s){if(c==L'\\'){++slashes;continue;}if(c==L'\"'){out.append(slashes*2+1,L'\\');out+=c;}else{out.append(slashes,L'\\');out+=c;}slashes=0;}out.append(slashes*2,L'\\');return out+L"\"";};
        std::wstring cmd=quote(executable.wstring());
        if(!devApp.empty())cmd+=L" "+quote(std::filesystem::path(devApp).wstring());
        cmd+=L" --bridge-dir "+quote(directory.wstring())+L" --owner-pid "+std::to_wstring(owner);
        STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
        if(CreateProcessW(executable.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,root.c_str(),&si,&pi)){process=pi.hProcess;CloseHandle(pi.hThread);}
#else
        std::vector<std::string> args={executable.string()};if(!devApp.empty())args.push_back(devApp);
        args.insert(args.end(),{"--bridge-dir",directory.string(),"--owner-pid",std::to_string(owner)});
        std::vector<char*> argv;for(auto &a:args)argv.push_back(a.data());argv.push_back(nullptr);
        std::vector<std::string> env;
        for(char **p=environ;*p;++p){std::string value=*p;if(value.rfind("LD_LIBRARY_PATH=",0)&&value.rfind("ELECTRON_RUN_AS_NODE=",0))env.push_back(value);}
        std::vector<char*> envp;for(auto &e:env)envp.push_back(e.data());envp.push_back(nullptr);
        pid_t pid=-1;if(posix_spawn(&pid,executable.c_str(),nullptr,nullptr,argv.data(),envp.data())==0)process=pid;
#endif
        if(!alive()){fields={{"state","error"},{"message","Could not start the microphone helper. Check its permissions."}};return false;}
        fields={{"state","starting"},{"message","Starting microphone connection..."}};
        heartbeat.clear();lastHeartbeat=lastPoll=0;sequence=0;
        return true;
    }
    bool command(const std::string &action){
        if(!alive())return false;
        std::ostringstream name;name<<"cmd-"<<std::setw(8)<<std::setfill('0')<<++sequence;
        auto target=directory/name.str(),temporary=target;temporary+=".tmp";
        {std::ofstream out(temporary);out<<action<<'\n';out.close();if(!out)return false;}
        std::error_code ec;std::filesystem::rename(temporary,target,ec);return !ec;
    }
    void shutdown(){
        if(alive()){
            command("quit");
            // Shutdown remains bounded even if the network/helper stalls.
            for(int i=0;i<25&&alive();++i)SDL_Delay(10);
            if(alive()){
#ifdef _WIN32
                TerminateProcess(process,0);CloseHandle(process);process=nullptr;
#else
                auto pid=process;kill(pid,SIGTERM);process=-1;
                std::thread([pid](){waitpid(pid,nullptr,0);}).detach();
#endif
            }
        }
        if(!directory.empty()){std::error_code ec;std::filesystem::remove_all(directory,ec);directory.clear();}
    }
};
