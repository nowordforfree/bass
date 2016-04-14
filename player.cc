#include <iostream>
#include <fstream>
#include "player.h"
#include "bass.h"
#ifdef _WIN32
  #include <windows.h>
  #ifdef _WIN64
    #define BASS_PATH "./win/x64/bass.dll"
  #else
    #define BASS_PATH "./win/bass.dll"
  #endif
#elif __linux__
  #include <dlfcn.h>
  #if defined(__amd64__) || defined(__x86_64__)
    #define BASS_PATH "./linux/x64/libbass.so"
  #else
    #define BASS_PATH "./linux/libbass.so"
  #endif
#endif

namespace bassplayer
{
  using v8::Function;
  using v8::FunctionCallbackInfo;
  using v8::FunctionTemplate;
  using v8::Isolate;
  using v8::Local;
  using v8::Number;
  using v8::Object;
  using v8::Persistent;
  using v8::String;
  using v8::Value;
  
  Persistent<Function> BASSPlayer::constructor;

  BASSPlayer::BASSPlayer()
  {
    #ifdef _WIN32
      handl = (void*)LoadLibrary(BASS_PATH);
      if (!handl)
      {
        std::cerr << "Failed to load library " << BASS_PATH << std::endl;
        return;
      }  
    #elif __linux__
      handl = dlopen (BASS_PATH, RTLD_LAZY);
      if (!handl)
      {
        std::cerr << "Failed to load library " << BASS_PATH << std::endl;
        std::cerr << dlerror() << std::endl;
        return;
      }
    #endif    
  }

  BASSPlayer::~BASSPlayer()
  {
    typedef bool (*BASS_Free)();
    BASS_Free bFree = this->LoadFunction<BASS_Free>("BASS_Free");
    bFree();
    #ifdef _WIN32
    FreeLibrary((HINSTANCE)handl);
    #elif __linux__
    dlclose(handl);
    #endif    
  }

  void BASSPlayer::Init(Local<Object> exports)
  {
    Isolate* isolate = exports-> GetIsolate();

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "BASSPlayer"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "Play", Play);
    // NODE_SET_PROTOTYPE_METHOD(tpl, "Pause", Pause);
    // NODE_SET_PROTOTYPE_METHOD(tpl, "Stop", Stop);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "BASSPlayer"), tpl->GetFunction());
  }

  void BASSPlayer::New(const FunctionCallbackInfo<Value>& args)
  {
    Isolate* isolate = args.GetIsolate();

    if (args.IsConstructCall())
    {
      BASSPlayer* player = new BASSPlayer();
      player->Wrap(args.This());
      args.GetReturnValue().Set(args.This());
    }
    else
    {
      const int argc = 1;
      Local<Value> argv[argc] = { args[0] };
      Local<Function> cons = Local<Function>::New(isolate, constructor);
      args.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }
  }

  void BASSPlayer::Play(const FunctionCallbackInfo<Value>& args)
  {
    
  }
}