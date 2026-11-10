#include "../src/video.hpp"
#include <cassert>
struct Reader final:IMFSourceReader {
    ULONG refs=1;unsigned reads=0;HRESULT nextResult=S_OK;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void **out)override{if(!out)return E_POINTER;*out=nullptr;return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
    ULONG STDMETHODCALLTYPE Release()override{return --refs;}
    HRESULT STDMETHODCALLTYPE GetStreamSelection(DWORD,BOOL*)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetStreamSelection(DWORD,BOOL)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetNativeMediaType(DWORD,DWORD,IMFMediaType**)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetCurrentMediaType(DWORD,IMFMediaType**)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetCurrentMediaType(DWORD,DWORD*,IMFMediaType*)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetCurrentPosition(REFGUID,REFPROPVARIANT)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE ReadSample(DWORD,DWORD,DWORD*,DWORD*,LONGLONG*,IMFSample**)override{++reads;return nextResult;}
    HRESULT STDMETHODCALLTYPE Flush(DWORD)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE GetServiceForStream(DWORD,REFGUID,REFIID,void**)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetPresentationAttribute(DWORD,REFGUID,PROPVARIANT*)override{return E_NOTIMPL;}
};
static Com<IMFSample> sample(BYTE value){
    Com<IMFSample> s;assert(SUCCEEDED(MFCreateSample(s.put())));
    Com<IMFMediaBuffer> b;assert(SUCCEEDED(MFCreateMemoryBuffer(1920*1080*2,b.put())));
    BYTE *bytes=nullptr;DWORD cap=0,size=0;assert(SUCCEEDED(b->Lock(&bytes,&cap,&size)));std::memset(bytes,value,cap);b->Unlock();b->SetCurrentLength(cap);assert(SUCCEEDED(s->AddBuffer(b.get())));return s;
}
int main(){
    CapturePlatform platform;Reader reader;SharedVideo shared;auto state=std::make_shared<WinReaderState>();
    state->target=&shared;state->reader=&reader;state->mode.name="yuyv";state->mode.stride=3840;state->running=true;
    Com<WinReaderCallback> callback(new WinReaderCallback(state));
    auto first=sample(16),second=sample(64);
    callback->OnReadSample(S_OK,0,0,0,first.get());assert(reader.reads==1&&state->pending&&shared.captured==0);
    first.reset(); // callback owns a sample reference after MF returns
    callback->OnReadSample(S_OK,0,0,166667,second.get());second.reset();
    assert(reader.reads==2&&state->stats.samples==2&&state->stats.pendingDropped==1);
    assert(state->decode(state->pending.get())&&state->frame.data[0]==64);state->pending.reset();
    // Null stream ticks rearm without being counted as a frame.
    callback->OnReadSample(S_OK,0,MF_SOURCE_READERF_STREAMTICK,333334,nullptr);assert(reader.reads==3&&state->stats.samples==2);
    state->running=false;callback->OnReadSample(S_OK,0,0,0,nullptr);assert(reader.reads==3);
    state->running=true;reader.nextResult=E_FAIL;callback->OnReadSample(S_OK,0,0,0,nullptr);assert(!state->running&&!shared.error.empty());
    std::cout<<"PASS native MF callback: immediate rearm, deferred decode, bounded latest sample, COM sample lifetime, stream ticks, stop and read failure\n";
}
