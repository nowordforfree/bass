#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stack>
#include <dirent.h>
#include "bass.h"
#include "player.h"
#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
  #define GetCurrentDir _getcwd
  #ifdef _WIN64
    std::string PluginsDir = std::string("win/x64/plugins/");
  #else
    std::string PluginsDir = std::string("win/ia32/plugins/");
  #endif
#else
  #include <unistd.h>
  #define GetCurrentDir getcwd
  #ifdef __linux__
    #if defined(__amd64__) || defined(__x86_64__)
      std::string PluginsDir = std::string("linux/x64/plugins/");
    #else
      std::string PluginsDir = std::string("linux/ia32/plugins/");
    #endif
  #elif defined __APPLE__
    std::string PluginsDir = std::string("mac/plugins/");
  #endif
#endif

using namespace v8;

namespace bassplayer
{
  bool FileExists(const char *fname)
  {
    if (FILE *file = fopen(fname, "r")) {
      fclose(file);
      return true;
    }
    else {
      return false;
    }
  }

  std::stack<std::string> listdirfiles (std::string dir)
  {
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
      std::cout << "Error(" << errno << ") opening " << dir << std::endl;
      return std::stack<std::string>();
    }

    std::stack<std::string> files = std::stack<std::string>();
    while ((dirp = readdir(dp)) != NULL) {
      if (dirp->d_name[0] != '.') {
        files.push(std::string(dirp->d_name));
      }
    }
    closedir(dp);
    return files;
  }

  BASSPlayer::BASSPlayer()
  {
    
    if (!BASS_Init(-1, 44100, 0, 0, 0))
    {
      std::cerr << "BASS_Init error" << std::endl;
      std::cerr << "Error code: " << BASS_ErrorGetCode() << std::endl;
      return;
    }

    BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 2);
    BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 30000);
    
    std::stack<std::string> pluginFilesStack = listdirfiles(PluginsDir);
    while (!pluginFilesStack.empty())
    {
      std::string pluginNameString = PluginsDir + pluginFilesStack.top();
      char * pluginName = new char[pluginNameString.length() + 1];
      std::copy(pluginNameString.begin(), pluginNameString.end(), pluginName);
      pluginName[pluginNameString.size()] = '\0';
      if (FileExists(pluginName))
      {
        std::cout << "File " << pluginName << " exists" << std::endl;
      }

      std::cout << "Trying to load plugin " << pluginName << std::endl;
      HPLUGIN pluginHandle = BASS_PluginLoad(pluginName, 0);
      if (pluginHandle == 0)
      {
        std::cerr << "Error occured when loading plugin " << pluginName << std::endl;
        std::cerr << "Error code: " << BASS_ErrorGetCode() << std::endl;
      }
      else
      {
        plugins.push(pluginHandle);
      }
      pluginFilesStack.pop();
    }
  }

  BASSPlayer::~BASSPlayer()
  {
    while(!plugins.empty())
    {
      BASS_PluginFree(this->plugins.top());
      plugins.pop();      
    }
    BASS_Free();
  }

  void BASSPlayer::Init(Local<Object> exports)
  {
    Isolate* isolate = exports->GetIsolate();

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "BASSPlayer"));
    tpl->InstanceTemplate()->SetInternalFieldCount(2);

    NODE_SET_PROTOTYPE_METHOD(tpl, "play", Play);
    NODE_SET_PROTOTYPE_METHOD(tpl, "pause", Pause);
    NODE_SET_PROTOTYPE_METHOD(tpl, "stop", Stop);

    Persistent<Function> constructor;
    constructor.Reset(isolate, tpl->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "BASSPlayer"), tpl->GetFunction());
  }

  void BASSPlayer::New(const FunctionCallbackInfo<Value>& args)
  {
    Isolate* isolate = args.GetIsolate();
    if (!args.IsConstructCall())
    {
      isolate->ThrowException(String::NewFromUtf8(isolate, "Use the new operator"));
      return;
    }

    BASSPlayer* player = new BASSPlayer();
    player->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  }

  void BASSPlayer::Play(const FunctionCallbackInfo<Value>& args)
  {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);

    BASSPlayer* bass = ObjectWrap::Unwrap<BASSPlayer>(args.Holder());

    if (!args[0]->IsUndefined())
    {
      std::string str_filePath;
      String::Utf8Value arg_string(args[0]->ToString());
      str_filePath = std::string(*arg_string);
      char * filePath = new char[str_filePath.length() + 1];
      std::copy(str_filePath.begin(), str_filePath.end(), filePath);
      filePath[str_filePath.size()] = '\0';

      if (str_filePath.length() > 0)
      {
        if (bass->stream != 0 && BASS_ChannelIsActive(bass->stream) == BASS_ACTIVE_PLAYING)
        {
          bass->Stop(args);
        }
        if (FileExists(filePath))
        {
          bass->stream = BASS_StreamCreateFile(FALSE, (void*)filePath, 0, 0, 0);
        }
        else
        {
          // Preprocess(filePath)
          // if that is playlist file -
          // parse into separate link and play each of them
          bass->stream = BASS_StreamCreateURL(filePath, 0, BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, NULL, 0);
        }
        if (!bass->stream)
        {
          std::cerr << "Error creating stream" << std::endl;
          std::cerr << "Error code: " << BASS_ErrorGetCode() << std::endl;
          args.GetReturnValue().Set(
            String::NewFromUtf8(
              isolate,
              "Error occured when tried to create a stream"
              )
            );
          return;
        }
        
        if (!BASS_ChannelPlay(bass->stream, FALSE))
        {
          std::cerr << "Error starting playback" << std::endl;
          std::cerr << "Error code: " << BASS_ErrorGetCode() << std::endl;
          args.GetReturnValue().Set(
            String::NewFromUtf8(
              isolate,
              "Error starting playback"
              )
            );
          return;
        }

        std::string message ("Playing");
        BASS_CHANNELINFO info;
        BASS_ChannelGetInfo(bass->stream, &info);
        if (info.ctype == BASS_CTYPE_STREAM_MP3)
        {
          message.append(" mp3 stream.");
        }
        TAG_ID3 *tags = (TAG_ID3*)BASS_ChannelGetTags(bass->stream, BASS_TAG_ID3);
        if (tags)
        {
          message += ' ' + tags->artist;
          message.append(" - ");
          message += tags->title;
        }

        delete [] filePath;
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, message.c_str()));
      }
      else
      {
        std::cerr << "Empty filePath parameter" << std::endl;
        args.GetReturnValue().Set(
          String::NewFromUtf8(
            isolate,
            "Empty filePath parameter"
            )
          );
        return;
      }
    }
    else
    {
      if (bass->stream != 0)
      {
        if (!BASS_ChannelPlay(bass->stream, FALSE))
        {
          std::cerr << "Error resuming playback" << std::endl;
          std::cerr << "Error code: " << BASS_ErrorGetCode() << std::endl;
          args.GetReturnValue().Set(
            String::NewFromUtf8(
              isolate,
              "Error resuming playback"
              )
            );
        }
      }
      else
      {
        args.GetReturnValue().Set(
          String::NewFromUtf8(
            isolate,
            "Nothing to play"
            )
          );
      }
    }
  }
  void BASSPlayer::Pause(const FunctionCallbackInfo<Value>& args)
  {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);

    BASSPlayer* bass = ObjectWrap::Unwrap<BASSPlayer>(args.Holder());

    if (!BASS_ChannelPause(bass->stream))
    {
      std::cerr << "Error pausing stream" << std::endl;
      std::cerr << "Error code: " << BASS_ErrorGetCode() << std::endl;
      args.GetReturnValue().Set(
        String::NewFromUtf8(
          isolate,
          "Error occured when tried to pause a stream"
          )
        );
    }
  }
  void BASSPlayer::Stop(const FunctionCallbackInfo<Value>& args)
  {
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);

    BASSPlayer* bass = ObjectWrap::Unwrap<BASSPlayer>(args.Holder());

    if (!BASS_ChannelStop(bass->stream))
    {
      std::cerr << "Error stopping stream" << std::endl;
      std::cerr << "Error code: " << BASS_ErrorGetCode() << std::endl;
      args.GetReturnValue().Set(
        String::NewFromUtf8(
          isolate,
          "Error occured when tried to stop a stream"
          )
        );
    }
    bass->stream = 0;
  }
}
