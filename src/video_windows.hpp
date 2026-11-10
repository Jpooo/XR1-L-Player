#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <condition_variable>
#include <memory>
#include <iomanip>
template<class T> class Com {
    T *pointer=nullptr;
public:
    Com()=default;explicit Com(T *p):pointer(p){}
    Com(const Com &other):pointer(other.pointer){if(pointer)pointer->AddRef();}
    Com &operator=(const Com &other){if(this!=&other){reset();pointer=other.pointer;if(pointer)pointer->AddRef();}return *this;}
    ~Com(){reset();}void reset(){if(pointer){pointer->Release();pointer=nullptr;}}
    T **put(){reset();return &pointer;}T *get()const{return pointer;}T *operator->()const{return pointer;}explicit operator bool()const{return pointer!=nullptr;}
};
inline std::string utf8(const wchar_t *s){if(!s)return {};int size=WideCharToMultiByte(CP_UTF8,0,s,-1,nullptr,0,nullptr,nullptr);if(size<1)return {};std::string result(static_cast<size_t>(size),'\0');WideCharToMultiByte(CP_UTF8,0,s,-1,result.data(),size,nullptr,nullptr);result.pop_back();return result;}
inline std::wstring wide(const std::string &s){int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);if(!size&& !s.empty())throw std::runtime_error("Invalid UTF-8 device identifier");std::wstring result(static_cast<size_t>(size),L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),result.data(),size);return result;}
inline std::string hresult(HRESULT hr){std::ostringstream s;s<<"0x"<<std::hex<<std::uppercase<<static_cast<unsigned long>(hr);return s.str();}
inline void mfCheck(HRESULT hr,const char *action){if(FAILED(hr))throw std::runtime_error(std::string(action)+" ("+hresult(hr)+"). Close other capture apps; check Windows camera access for desktop apps and the USB connection.");}
struct CapturePlatform {
    bool initializedCom=false,initializedMf=false;
    CapturePlatform(){HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);initializedCom=SUCCEEDED(hr);if(FAILED(hr)&&hr!=RPC_E_CHANGED_MODE)mfCheck(hr,"COM initialization failed");hr=MFStartup(MF_VERSION,MFSTARTUP_NOSOCKET);if(FAILED(hr)){if(initializedCom)CoUninitialize();mfCheck(hr,"Media Foundation initialization failed; Windows N editions require the Media Feature Pack");}initializedMf=true;}
    ~CapturePlatform(){if(initializedMf)MFShutdown();if(initializedCom)CoUninitialize();}
};
inline std::vector<VideoDevice> videoDevices(){
    Com<IMFAttributes> attrs;mfCheck(MFCreateAttributes(attrs.put(),1),"Cannot create device enumeration");attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    IMFActivate **devices=nullptr;UINT32 count=0;mfCheck(MFEnumDeviceSources(attrs.get(),&devices,&count),"Cannot enumerate video devices");
    std::vector<VideoDevice> result;
    for(UINT32 i=0;i<count;++i){wchar_t *name=nullptr,*id=nullptr;UINT32 n=0;devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,&name,&n);devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,&id,&n);if(id){std::string path=utf8(id),usb;auto l=lower(path);auto v=l.find("vid_"),p=l.find("pid_");if(v!=std::string::npos&&p!=std::string::npos&&v+8<=l.size()&&p+8<=l.size())usb=l.substr(v+4,4)+":"+l.substr(p+4,4);result.push_back({path,utf8(name),usb});}CoTaskMemFree(name);CoTaskMemFree(id);devices[i]->Release();}
    CoTaskMemFree(devices);return result;
}
inline bool sameVideoPath(const std::string &a,const std::string &b){return lower(a)==lower(b);}
inline Com<IMFMediaSource> openMfSource(const std::string &path){Com<IMFAttributes> attrs;mfCheck(MFCreateAttributes(attrs.put(),2),"Cannot configure camera");attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);auto id=wide(path);attrs->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,id.c_str());Com<IMFMediaSource> source;mfCheck(MFCreateDeviceSource(attrs.get(),source.put()),"Cannot open camera");return source;}
inline std::string mfFormatName(const GUID &g){if(g==MFVideoFormat_YUY2)return "yuyv";if(g==MFVideoFormat_NV12)return "nv12";if(g==MFVideoFormat_UYVY)return "uyvy";if(g==MFVideoFormat_I420||g==MFVideoFormat_IYUV)return "i420";if(g==MFVideoFormat_YV12)return "yv12";if(g==MFVideoFormat_MJPG)return "mjpeg";return "other";}
inline std::vector<DWORD> videoStreams(IMFMediaSource *source){Com<IMFPresentationDescriptor> pd;DWORD count=0;if(SUCCEEDED(source->CreatePresentationDescriptor(pd.put()))&&SUCCEEDED(pd->GetStreamDescriptorCount(&count))&&count<=64){std::vector<DWORD> streams;for(DWORD i=0;i<count;++i){BOOL selected;Com<IMFStreamDescriptor> sd;Com<IMFMediaTypeHandler> handler;GUID major{};if(SUCCEEDED(pd->GetStreamDescriptorByIndex(i,&selected,sd.put()))&&SUCCEEDED(sd->GetMediaTypeHandler(handler.put()))&&SUCCEEDED(handler->GetMajorType(&major))&&major==MFMediaType_Video)streams.push_back(i);}if(!streams.empty())return streams;}return {MF_SOURCE_READER_FIRST_VIDEO_STREAM};}
struct WinMode {Com<IMFMediaType> type;DWORD stream;GUID subtype;std::string name;UINT32 fpsN=0,fpsD=0;LONG stride=0;int rank=99;};
inline std::vector<WinMode> windowsModes(IMFSourceReader *reader,IMFMediaSource *source,const std::string &requested){
    std::vector<WinMode> result;const std::vector<std::string> order={"yuyv","nv12","uyvy","i420","yv12","mjpeg"};
    for(auto stream:videoStreams(source))for(DWORD index=0;index<2048;++index){
        Com<IMFMediaType> type;HRESULT hr=reader->GetNativeMediaType(stream,index,type.put());if(FAILED(hr))break;
        UINT32 w=0,h=0,n=0,d=0,interlace=MFVideoInterlace_Progressive;GUID subtype{};
        if(FAILED(MFGetAttributeSize(type.get(),MF_MT_FRAME_SIZE,&w,&h))||w!=1920||h!=1080||FAILED(MFGetAttributeRatio(type.get(),MF_MT_FRAME_RATE,&n,&d))||!rate60(n,d)||FAILED(type->GetGUID(MF_MT_SUBTYPE,&subtype)))continue;
        type->GetUINT32(MF_MT_INTERLACE_MODE,&interlace);if(interlace!=MFVideoInterlace_Progressive&&interlace!=MFVideoInterlace_Unknown)continue;
        auto name=mfFormatName(subtype);auto pos=std::find(order.begin(),order.end(),name);if(pos==order.end()||(requested!="auto"&&name!=requested))continue;
        UINT32 stride=0;type->GetUINT32(MF_MT_DEFAULT_STRIDE,&stride);WinMode mode{type,stream,subtype,name,n,d,static_cast<LONG>(stride),static_cast<int>(pos-order.begin())};result.push_back(mode);
    }
    std::stable_sort(result.begin(),result.end(),[](const auto &a,const auto &b){return a.rank<b.rank;});return result;
}
struct WinReaderState {
    std::mutex mutex;std::atomic<bool> running{false};SharedVideo *target=nullptr;IMFSourceReader *reader=nullptr;WinMode mode;Frame frame;JpegDecoder jpeg;double lastValid=0;
    void fail(const std::string &error){running=false;if(target)target->fail(error);}
    bool decode(IMFSample *sample){
        Com<IMFMediaBuffer> buffer;if(FAILED(sample->ConvertToContiguousBuffer(buffer.put())))return false;
        bool compressed=mode.name=="mjpeg";RawFormat raw=mode.name=="yuyv"?RawFormat::YUYV:mode.name=="uyvy"?RawFormat::UYVY:mode.name=="i420"?RawFormat::I420:mode.name=="yv12"?RawFormat::YV12:RawFormat::NV12;
        if(!compressed){
            Com<IMF2DBuffer2> two;
            if(SUCCEEDED(buffer->QueryInterface(__uuidof(IMF2DBuffer2),reinterpret_cast<void**>(two.put())))){
                BYTE *scan=nullptr,*base=nullptr;LONG pitch=0;DWORD length=0;
                if(SUCCEEDED(two->Lock2DSize(MF2DBuffer_LockFlags_Read,&scan,&pitch,&base,&length))){bool okay=pitch>0&&scan>=base&&static_cast<size_t>(scan-base)<=length&&unpackRaw(scan,length-static_cast<size_t>(scan-base),static_cast<size_t>(pitch),raw,frame);two->Unlock2D();return okay;}
            }
        }
        BYTE *bytes=nullptr;DWORD capacity=0,size=0;if(FAILED(buffer->Lock(&bytes,&capacity,&size)))return false;
        bool okay=size<=capacity&&(compressed?jpeg.decode(bytes,size,frame):(mode.stride>=0&&unpackRaw(bytes,size,static_cast<size_t>(mode.stride),raw,frame)));buffer->Unlock();return okay;
    }
};
class WinReaderCallback final:public IMFSourceReaderCallback {
    std::atomic<ULONG> references{1};std::shared_ptr<WinReaderState> state;
public:
    explicit WinReaderCallback(std::shared_ptr<WinReaderState> s):state(std::move(s)){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **out)override{if(!out)return E_POINTER;*out=nullptr;if(iid==__uuidof(IUnknown)||iid==__uuidof(IMFSourceReaderCallback)){*out=static_cast<IMFSourceReaderCallback*>(this);AddRef();return S_OK;}return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++references;}
    ULONG STDMETHODCALLTYPE Release()override{ULONG n=--references;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE OnReadSample(HRESULT hr,DWORD,DWORD flags,LONGLONG,IMFSample *sample)override{
        Com<IMFSourceReader> next;DWORD stream=0;
        {
            std::lock_guard<std::mutex> lock(state->mutex);if(!state->running||!state->target)return S_OK;
            if(FAILED(hr)){state->fail("Windows capture stopped ("+hresult(hr)+"). Check the card, USB connection and camera permissions.");return S_OK;}
            if(flags&MF_SOURCE_READERF_ENDOFSTREAM){state->fail("The capture device stopped streaming or was disconnected.");return S_OK;}
            if(flags&(MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED|MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)){
                Com<IMFMediaType> current;UINT32 w=0,h=0,n=0,d=0;GUID subtype{};
                if(FAILED(state->reader->GetCurrentMediaType(state->mode.stream,current.put()))||FAILED(MFGetAttributeSize(current.get(),MF_MT_FRAME_SIZE,&w,&h))||w!=1920||h!=1080||FAILED(MFGetAttributeRatio(current.get(),MF_MT_FRAME_RATE,&n,&d))||!rate60(n,d)||FAILED(current->GetGUID(MF_MT_SUBTYPE,&subtype))||subtype!=state->mode.subtype){state->fail("The camera changed its capture format. Stop and restart the preview.");return S_OK;}
            }
            if(sample){bool valid=state->decode(sample);if(valid){state->frame.received=monotime();state->lastValid=state->frame.received;state->target->publish(state->frame);}else ++state->target->corrupt;}
            if(state->reader){state->reader->AddRef();next=Com<IMFSourceReader>(state->reader);stream=state->mode.stream;}
        }
        if(next){hr=next->ReadSample(stream,0,nullptr,nullptr,nullptr,nullptr);if(FAILED(hr)){std::lock_guard<std::mutex> lock(state->mutex);if(state->running)state->fail("Cannot request the next camera frame ("+hresult(hr)+").");}}
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnFlush(DWORD)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE OnEvent(DWORD,IMFMediaEvent *event)override{HRESULT status=S_OK;if(event&&SUCCEEDED(event->GetStatus(&status))&&FAILED(status)){std::lock_guard<std::mutex> lock(state->mutex);if(state->running)state->fail("Camera event failed ("+hresult(status)+").");}return S_OK;}
};
class Capture {
    SharedVideo &shared;Com<IMFMediaSource> source;Com<IMFSourceReader> reader;std::shared_ptr<WinReaderState> state;std::thread watchdog;
public:
    explicit Capture(SharedVideo &s):shared(s){}~Capture(){stop();}
    void start(const std::string &path,const std::string &requested,unsigned=2){
        stop();{std::lock_guard<std::mutex> lock(shared.mutex);shared.error.clear();shared.dirty=false;shared.captured=0;shared.skipped=0;shared.corrupt=0;}
        try{
            source=openMfSource(path);state=std::make_shared<WinReaderState>();Com<WinReaderCallback> callback(new WinReaderCallback(state));Com<IMFAttributes> attrs;mfCheck(MFCreateAttributes(attrs.put(),4),"Cannot configure source reader");
            attrs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,callback.get());attrs->SetUINT32(MF_LOW_LATENCY,TRUE);attrs->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS,TRUE);attrs->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING,FALSE);
            mfCheck(MFCreateSourceReaderFromMediaSource(source.get(),attrs.get(),reader.put()),"Cannot create native capture reader");
            auto modes=windowsModes(reader.get(),source.get(),requested);bool selected=false;HRESULT last=MF_E_INVALIDMEDIATYPE;
            for(auto &mode:modes){reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,FALSE);last=reader->SetCurrentMediaType(mode.stream,nullptr,mode.type.get());if(FAILED(last))continue;last=reader->SetStreamSelection(mode.stream,TRUE);if(FAILED(last))continue;state->mode=mode;selected=true;break;}
            if(!selected)throw std::runtime_error("No supported native 1920x1080 at 60/59.94 fps mode ("+hresult(last)+"). Use USB 3, Xbox 1080p/60 SDR; check --diagnose and the XR1 Lite firmware.");
            state->target=&shared;state->reader=reader.get();state->lastValid=monotime();state->running=true;
            std::ostringstream mode;mode<<"1920 x 1080 / "<<double(state->mode.fpsN)/state->mode.fpsD<<" fps / "<<state->mode.name<<" / Media Foundation native";shared.mode=mode.str();
            mfCheck(reader->ReadSample(state->mode.stream,0,nullptr,nullptr,nullptr,nullptr),"Cannot start native capture");
            auto active=state;watchdog=std::thread([active](){while(active->running){std::this_thread::sleep_for(std::chrono::milliseconds(100));std::lock_guard<std::mutex> lock(active->mutex);if(active->running&&monotime()-active->lastValid>5)active->fail("No valid video frames for 5 seconds. Check HDMI, USB 3 and Xbox 1080p/60 settings.");}});
        }catch(...){stop();throw;}
    }
    void stop(){
        if(state){{std::lock_guard<std::mutex> lock(state->mutex);state->running=false;state->target=nullptr;state->reader=nullptr;}if(watchdog.joinable())watchdog.join();}
        if(reader)reader->Flush(MF_SOURCE_READER_ALL_STREAMS);
        if(source)source->Shutdown();
        reader.reset();source.reset();state.reset();
    }
};
inline void diagnoseVideo(){
    auto devices=videoDevices();if(devices.empty())std::cout<<"No Media Foundation video devices. Check USB, camera permissions and the Media Feature Pack on Windows N.\n";
    for(auto &device:devices){std::cout<<device.name<<" / USB "<<device.usbId<<"\n  ID: "<<device.path<<"\n";try{auto source=openMfSource(device.path);Com<IMFSourceReader> reader;Com<IMFAttributes> attrs;MFCreateAttributes(attrs.put(),1);attrs->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS,TRUE);mfCheck(MFCreateSourceReaderFromMediaSource(source.get(),attrs.get(),reader.put()),"Cannot inspect capture modes");for(auto stream:videoStreams(source.get()))for(DWORD i=0;i<2048;++i){Com<IMFMediaType> type;if(FAILED(reader->GetNativeMediaType(stream,i,type.put())))break;UINT32 w=0,h=0,n=0,d=0;GUID sub{};if(SUCCEEDED(MFGetAttributeSize(type.get(),MF_MT_FRAME_SIZE,&w,&h))&&w==1920&&h==1080&&SUCCEEDED(MFGetAttributeRatio(type.get(),MF_MT_FRAME_RATE,&n,&d))&&d&&SUCCEEDED(type->GetGUID(MF_MT_SUBTYPE,&sub)))std::cout<<"  stream "<<stream<<": "<<mfFormatName(sub)<<" 1920x1080 @ "<<double(n)/d<<" fps\n";}reader.reset();source->Shutdown();}catch(const std::exception &e){std::cout<<"  "<<e.what()<<"\n";}}
}
