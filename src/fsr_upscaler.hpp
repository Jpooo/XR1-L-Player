#pragma once
#include "core.hpp"
#include <SDL_opengl.h>
#include <stdexcept>
inline const char *fsrSharpKey(unsigned level){static const char *keys[]={"off","low","medium","high"};return keys[level<4?level:0];}
inline const char *fsrSharpLabel(unsigned level){static const char *labels[]={"Off","Low","Medium","High"};return labels[level<4?level:0];}
// AMD FP32 EASU with an optional RCAS pass. No temporal history or frame queue.
class FsrUpscaler {
#define FSR_GL(ret,name,args) using name##Fn=ret(APIENTRY*)args;name##Fn name=nullptr;
    FSR_GL(void,GetIntegerv,(GLenum,GLint*))
    FSR_GL(GLuint,CreateShader,(GLenum))
    FSR_GL(void,ShaderSource,(GLuint,GLsizei,const GLchar*const*,const GLint*))
    FSR_GL(void,CompileShader,(GLuint))
    FSR_GL(void,GetShaderiv,(GLuint,GLenum,GLint*))
    FSR_GL(void,GetShaderInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*))
    FSR_GL(void,DeleteShader,(GLuint))
    FSR_GL(GLuint,CreateProgram,())
    FSR_GL(void,AttachShader,(GLuint,GLuint))
    FSR_GL(void,LinkProgram,(GLuint))
    FSR_GL(void,GetProgramiv,(GLuint,GLenum,GLint*))
    FSR_GL(void,GetProgramInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*))
    FSR_GL(void,DeleteProgram,(GLuint))
    FSR_GL(void,UseProgram,(GLuint))
    FSR_GL(GLint,GetUniformLocation,(GLuint,const GLchar*))
    FSR_GL(void,Uniform1i,(GLint,GLint))
    FSR_GL(void,Uniform1f,(GLint,GLfloat))
    FSR_GL(void,Uniform2f,(GLint,GLfloat,GLfloat))
    FSR_GL(void,Begin,(GLenum))
    FSR_GL(void,End,())
    FSR_GL(void,Vertex2f,(GLfloat,GLfloat))
#undef FSR_GL
    SDL_Texture *rgb=nullptr,*output=nullptr,*sharpened=nullptr;SDL_Renderer *owner=nullptr;
    GLuint program=0,rcasProgram=0;unsigned lastSharp=0;bool rcasAttempted=false,sharpValid=false,sharpAllocationFailed=false;int width=0,height=0;bool attempted=false,valid=false,allocationFailed=false;
    std::string message="Applies to video above 1080p.";
    static std::string read(const std::string &path){
        size_t bytes=0;void *data=SDL_LoadFile(path.c_str(),&bytes);
        if(!data)throw std::runtime_error("FSR shader assets missing");
        std::string source(static_cast<const char*>(data),bytes);SDL_free(data);return source;
    }
    GLuint shader(GLenum kind,const std::string &source){
        GLuint s=CreateShader(kind);const char *p=source.c_str();ShaderSource(s,1,&p,nullptr);CompileShader(s);GLint ok=0;GetShaderiv(s,GL_COMPILE_STATUS,&ok);
        if(!ok){char log[1024]{};GetShaderInfoLog(s,sizeof(log),nullptr,log);DeleteShader(s);throw std::runtime_error(std::string("FSR shader unsupported: ")+log);}return s;
    }
    bool initialize(SDL_Renderer *r,const std::string &assets){
        if(attempted)return program!=0;
        attempted=true;owner=r;
        SDL_RendererInfo info{};SDL_GetRendererInfo(r,&info);
        if(!info.name||std::string(info.name)!="opengl"){message="Restart app to enable the FSR OpenGL renderer.";return false;}
#define FSR_LOAD(name) name=reinterpret_cast<name##Fn>(SDL_GL_GetProcAddress("gl" #name));if(!name){message="OpenGL shader functions unavailable; using normal scaling.";return false;}
        FSR_LOAD(GetIntegerv) FSR_LOAD(CreateShader) FSR_LOAD(ShaderSource) FSR_LOAD(CompileShader) FSR_LOAD(GetShaderiv) FSR_LOAD(GetShaderInfoLog) FSR_LOAD(DeleteShader)
        FSR_LOAD(CreateProgram) FSR_LOAD(AttachShader) FSR_LOAD(LinkProgram) FSR_LOAD(GetProgramiv) FSR_LOAD(GetProgramInfoLog) FSR_LOAD(DeleteProgram) FSR_LOAD(UseProgram)
        FSR_LOAD(GetUniformLocation) FSR_LOAD(Uniform1i) FSR_LOAD(Uniform1f) FSR_LOAD(Uniform2f) FSR_LOAD(Begin) FSR_LOAD(End) FSR_LOAD(Vertex2f)
#undef FSR_LOAD
        GLuint vs=0,fs=0,p=0;
        try{
            vs=shader(GL_VERTEX_SHADER,"#version 420 compatibility\nvoid main(){gl_Position=gl_Vertex;}\n");
            std::string source="#version 420 compatibility\n#define A_GPU 1\n#define A_GLSL 1\n"+read(assets+"/fsr/ffx_a.h")+"\n#define FSR_EASU_F 1\n"+read(assets+"/fsr/ffx_fsr1.h")+R"(
uniform sampler2D inputImage;
uniform vec2 outputSize;
AF4 FsrEasuRF(AF2 p){return textureGather(inputImage,p,0);}
AF4 FsrEasuGF(AF2 p){return textureGather(inputImage,p,1);}
AF4 FsrEasuBF(AF2 p){return textureGather(inputImage,p,2);}
out vec4 resultColor;
void main(){
    AU4 c0,c1,c2,c3;
    FsrEasuCon(c0,c1,c2,c3,1920.0,1080.0,1920.0,1080.0,outputSize.x,outputSize.y);
    AF3 color;FsrEasuF(color,AU2(gl_FragCoord.xy),c0,c1,c2,c3);
    resultColor=vec4(color,1.0);
}
)";
            fs=shader(GL_FRAGMENT_SHADER,source);p=CreateProgram();AttachShader(p,vs);AttachShader(p,fs);LinkProgram(p);GLint ok=0;GetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char log[1024]{};GetProgramInfoLog(p,sizeof(log),nullptr,log);throw std::runtime_error(std::string("FSR link failed: ")+log);}
            program=p;DeleteShader(vs);DeleteShader(fs);return true;
        }catch(const std::exception &e){if(vs)DeleteShader(vs);if(fs)DeleteShader(fs);if(p)DeleteProgram(p);message=e.what();return false;}
    }
    bool initializeRcas(const std::string &assets){
        if(rcasAttempted)return rcasProgram!=0;
        rcasAttempted=true;GLuint vs=0,fs=0,p=0;
        try{
            vs=shader(GL_VERTEX_SHADER,"#version 420 compatibility\nvoid main(){gl_Position=gl_Vertex;}\n");
            std::string source="#version 420 compatibility\n#define A_GPU 1\n#define A_GLSL 1\n"+read(assets+"/fsr/ffx_a.h")+"\n#define FSR_RCAS_F 1\n"+read(assets+"/fsr/ffx_fsr1.h")+R"(
uniform sampler2D inputImage;
uniform float sharpnessStops;
AF4 FsrRcasLoadF(ASU2 p){return texelFetch(inputImage,clamp(ivec2(p),ivec2(0),textureSize(inputImage,0)-ivec2(1)),0);}
void FsrRcasInputF(inout AF1 r,inout AF1 g,inout AF1 b){}
out vec4 resultColor;
void main(){
    AU4 con;FsrRcasCon(con,sharpnessStops);
    AF1 r,g,b;FsrRcasF(r,g,b,AU2(gl_FragCoord.xy),con);
    resultColor=vec4(r,g,b,1.0);
}
)";
            fs=shader(GL_FRAGMENT_SHADER,source);p=CreateProgram();AttachShader(p,vs);AttachShader(p,fs);LinkProgram(p);GLint ok=0;GetProgramiv(p,GL_LINK_STATUS,&ok);
            if(!ok)throw std::runtime_error("RCAS shader could not be linked");
            rcasProgram=p;DeleteShader(vs);DeleteShader(fs);return true;
        }catch(const std::exception &){if(vs)DeleteShader(vs);if(fs)DeleteShader(fs);if(p)DeleteProgram(p);return false;}
    }
    bool sharpen(SDL_Renderer *r,unsigned level){
        if(sharpAllocationFailed)return false;
        if(!sharpened)sharpened=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,width,height);
        if(!sharpened){sharpAllocationFailed=true;return false;}
        if(sharpValid&&lastSharp==level)return true;
        auto *previous=SDL_GetRenderTarget(r);SDL_Rect viewport{},clip{};float sx=1,sy=1;SDL_RenderGetViewport(r,&viewport);SDL_RenderGetScale(r,&sx,&sy);bool clipped=SDL_RenderIsClipEnabled(r);SDL_RenderGetClipRect(r,&clip);
        auto restore=[&](){SDL_SetRenderTarget(r,previous);SDL_RenderSetViewport(r,&viewport);SDL_RenderSetScale(r,sx,sy);SDL_RenderSetClipRect(r,clipped?&clip:nullptr);};
        int seeded=SDL_SetRenderTarget(r,sharpened);SDL_RenderSetViewport(r,nullptr);SDL_RenderSetScale(r,1,1);SDL_RenderSetClipRect(r,nullptr);
        SDL_Rect seed{0,0,1,1};if(seeded==0)seeded=SDL_RenderCopy(r,output,nullptr,&seed);SDL_RenderFlush(r);
        if(seeded<0){restore();sharpAllocationFailed=true;return false;}
        GLint oldProgram=0;GetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);float tw=0,th=0;
        if(SDL_GL_BindTexture(output,&tw,&th)<0){restore();sharpAllocationFailed=true;return false;}
        UseProgram(rcasProgram);Uniform1i(GetUniformLocation(rcasProgram,"inputImage"),0);
        // AMD RCAS uses stops: two/one/zero stops map to Low/Medium/High.
        Uniform1f(GetUniformLocation(rcasProgram,"sharpnessStops"),float(3-level));
        Begin(GL_TRIANGLE_STRIP);Vertex2f(-1,-1);Vertex2f(1,-1);Vertex2f(-1,1);Vertex2f(1,1);End();UseProgram(GLuint(oldProgram));
        restore();sharpValid=true;lastSharp=level;return true;
    }

public:
    FsrUpscaler()=default;FsrUpscaler(const FsrUpscaler&)=delete;FsrUpscaler &operator=(const FsrUpscaler&)=delete;
    ~FsrUpscaler(){reset();}
    void reset(){if(rgb)SDL_DestroyTexture(rgb);if(output)SDL_DestroyTexture(output);if(sharpened)SDL_DestroyTexture(sharpened);if(rcasProgram&&DeleteProgram)DeleteProgram(rcasProgram);if(program&&DeleteProgram)DeleteProgram(program);rgb=output=sharpened=nullptr;program=rcasProgram=0;rcasAttempted=sharpValid=sharpAllocationFailed=false;lastSharp=0;owner=nullptr;attempted=valid=allocationFailed=false;width=height=0;message="Applies to video above 1080p.";}
    void invalidate(){valid=sharpValid=false;message="Applies to video above 1080p.";}
    const std::string &status()const{return message;}
    int render(SDL_Renderer *r,SDL_Texture *input,const SDL_Rect &dst,const std::string &assets,bool changed,unsigned sharpening=0){
        if(sharpening>3)sharpening=0;
        if(dst.w<=1920||dst.h<=1080){message="Native size: normal scaling.";return SDL_RenderCopy(r,input,nullptr,&dst);}
        if(!initialize(r,assets))return SDL_RenderCopy(r,input,nullptr,&dst);
        if(allocationFailed&&width==dst.w&&height==dst.h){message="FSR target unavailable; using normal scaling.";return SDL_RenderCopy(r,input,nullptr,&dst);}
        if(!rgb)rgb=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,1920,1080);
        if(width!=dst.w||height!=dst.h){if(output)SDL_DestroyTexture(output);if(sharpened)SDL_DestroyTexture(sharpened);output=sharpened=nullptr;sharpValid=sharpAllocationFailed=false;width=dst.w;height=dst.h;valid=false;allocationFailed=false;}
        if(!output)output=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,width,height);
        if(!rgb||!output){allocationFailed=true;message="FSR target unavailable; using normal scaling.";return SDL_RenderCopy(r,input,nullptr,&dst);}
        if(changed||!valid){
            valid=sharpValid=false;
            auto *previous=SDL_GetRenderTarget(r);SDL_Rect viewport{},clip{};float sx=1,sy=1;SDL_RenderGetViewport(r,&viewport);SDL_RenderGetScale(r,&sx,&sy);bool clipped=SDL_RenderIsClipEnabled(r);SDL_RenderGetClipRect(r,&clip);
            auto restore=[&](){SDL_SetRenderTarget(r,previous);SDL_RenderSetViewport(r,&viewport);SDL_RenderSetScale(r,sx,sy);SDL_RenderSetClipRect(r,clipped?&clip:nullptr);};
            SDL_BlendMode blend{};SDL_GetTextureBlendMode(input,&blend);SDL_SetTextureBlendMode(input,SDL_BLENDMODE_NONE);
            int converted=SDL_SetRenderTarget(r,rgb);SDL_RenderSetViewport(r,nullptr);SDL_RenderSetScale(r,1,1);SDL_RenderSetClipRect(r,nullptr);
            if(converted==0)converted=SDL_RenderCopy(r,input,nullptr,nullptr);
            SDL_SetTextureBlendMode(input,blend);
            // Seed SDL's RGB shader/state before using custom GL, including after NV12.
            if(converted==0)converted=SDL_SetRenderTarget(r,output);
            SDL_Rect seed{0,0,1,1};
            if(converted==0)converted=SDL_RenderCopy(r,rgb,nullptr,&seed);
            SDL_RenderFlush(r);
            if(converted<0){restore();message="FSR conversion unavailable; using normal scaling.";return SDL_RenderCopy(r,input,nullptr,&dst);}
            GLint oldProgram=0;GetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);
            float tw=0,th=0;
            if(SDL_GL_BindTexture(rgb,&tw,&th)<0){restore();message="FSR texture unavailable; using normal scaling.";return SDL_RenderCopy(r,input,nullptr,&dst);}
            UseProgram(program);Uniform1i(GetUniformLocation(program,"inputImage"),0);Uniform2f(GetUniformLocation(program,"outputSize"),float(width),float(height));
            Begin(GL_TRIANGLE_STRIP);Vertex2f(-1,-1);Vertex2f(1,-1);Vertex2f(-1,1);Vertex2f(1,1);End();
            UseProgram(GLuint(oldProgram)); // Leave the exact RGB state SDL cached intact.
            restore();valid=true;
        }
        if(sharpening){
            if(initializeRcas(assets)&&sharpen(r,sharpening)){
                message=std::string("FSR 1 active; sharpening: ")+fsrSharpLabel(sharpening)+".";
                return SDL_RenderCopy(r,sharpened,nullptr,&dst);
            }
            message="FSR 1 active; sharpening unavailable.";
        }else message="FSR 1 EASU active; sharpening off.";
        return SDL_RenderCopy(r,output,nullptr,&dst);
    }
};
