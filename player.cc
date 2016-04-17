#include <iostream>
#include <fstream>
#include <stdio.h>
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

using namespace v8;
namespace bassplayer
{
	/* start */
	typedef BOOL(*BASS_Init)(
		int device,
		DWORD freq,
		DWORD flags,
		DWORD win,
#ifdef _WIN32
		const GUID *dsguid
#else
		void *dsguid
#endif
		);
	typedef BOOL(*BASS_SetConfig)(
		DWORD option,
		DWORD value
		);
	typedef HSTREAM(*BASS_StreamCreateFile)(
		BOOL mem,
		void *file,
		QWORD offset,
		QWORD length,
		DWORD flags
		);
	typedef HSTREAM(*BASS_StreamCreateURL)(
		char *url,
		DWORD offset,
		DWORD flags,
		DOWNLOADPROC *proc,
		void *user
		);
	typedef BOOL(*BASS_ChannelPlay)(
		DWORD handle,
		BOOL restart
		);
	typedef BOOL(*BASS_ChannelPause)(
		DWORD handle
		);
	typedef BOOL(*BASS_ChannelStop) (
		DWORD handle
		);
	typedef BOOL(*BASS_StreamFree)(
		HSTREAM handle
		);
	typedef DWORD(*BASS_ChannelIsActive)(
		DWORD handle
		);
	//typedef BOOL(*BASS_Start)();
	//typedef BOOL(*BASS_Pause)();
	//typedef BOOL(*BASS_Stop)();
	typedef bool(*BASS_Free)();
	typedef int(*BASS_ErrorGetCode)();
	/* end */

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
		handl = dlopen(BASS_PATH, RTLD_LAZY);
		if (!handl)
		{
			std::cerr << "Failed to load library " << BASS_PATH << std::endl;
			std::cerr << dlerror() << std::endl;
			return;
		}
#endif

		BASS_Init bInit = this->LoadFunction<BASS_Init>("BASS_Init");
		BASS_SetConfig bSetConfig = this->LoadFunction<BASS_SetConfig>("BASS_SetConfig");
		bool init = bInit(-1, 44100, 0, 0, 0);
		if (!init)
		{
			std::cerr << "BASS_Init error" << std::endl;
			return;
		}
		bSetConfig(BASS_CONFIG_NET_PLAYLIST, 2);
		bSetConfig(BASS_CONFIG_NET_TIMEOUT, 30000);
	}

	BASSPlayer::~BASSPlayer()
	{
		BASS_Free bFree = this->LoadFunction<BASS_Free>("BASS_Free");
		bFree();
#ifdef _WIN32
		FreeLibrary((HINSTANCE)handl);
#elif __linux__
		dlclose(handl);
#endif    
	}

	template <typename T> T BASSPlayer::LoadFunction(const char *funcName)
	{
		char *error;
		T result;
#ifdef _WIN32
		result = (T)GetProcAddress((HINSTANCE)handl, funcName);
		if (!result)
		{
			std::cerr << "Failed to load function " << funcName << std::endl;
			return NULL;
		}
#elif __linux__
		result = (T)dlsym(handl, funcName);
		if ((error = dlerror()) != NULL)
		{
			std::cerr << error << std::endl;
			return NULL;
		}
#endif
		return result;
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

		BASS_StreamCreateFile bStreamFile = bass->LoadFunction<BASS_StreamCreateFile>("BASS_StreamCreateFile");
		BASS_StreamCreateURL bStreamUrl = bass->LoadFunction<BASS_StreamCreateURL>("BASS_StreamCreateURL");
		BASS_ChannelIsActive bChannelIsActive = bass->LoadFunction<BASS_ChannelIsActive>("BASS_ChannelIsActive");
		BASS_ChannelPlay bChannelPlay = bass->LoadFunction<BASS_ChannelPlay>("BASS_ChannelPlay");
		BASS_ErrorGetCode bGetErrorCode = bass->LoadFunction<BASS_ErrorGetCode>("BASS_ErrorGetCode");

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
				if (bass->stream != NULL && bChannelIsActive(bass->stream) == BASS_ACTIVE_PLAYING)
				{
					bass->Stop(args);
				}
				bool isStreamRemote = FileExists(filePath);
				if (isStreamRemote)
				{
					bass->stream = bStreamFile(FALSE, (void*)filePath, 0, 0, 0);
				}
				else
				{
					bass->stream = bStreamUrl(filePath, 0, BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, 0, 0);
				}
				if (!bass->stream)
				{
					std::cerr << "Error creating stream" << std::endl;
					std::cerr << "Error code: " << bGetErrorCode() << std::endl;
					args.GetReturnValue().Set(
						String::NewFromUtf8(
							isolate,
							"Error occured when tried to create a stream"
						)
					);
					return;
				}
				if (!bChannelPlay(bass->stream, FALSE))
				{
					std::cerr << "Error starting playback" << std::endl;
					std::cerr << "Error code: " << bGetErrorCode() << std::endl;
					args.GetReturnValue().Set(
						String::NewFromUtf8(
							isolate,
							"Error starting playback"
						)
					);
					return;
				}
				args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Playing"));
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
			if (bass->stream != NULL)
			{
				if (!bChannelPlay(bass->stream, FALSE))
				{
					std::cerr << "Error resuming playback" << std::endl;
					std::cerr << "Error code: " << bGetErrorCode() << std::endl;
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

		BASS_ChannelPause bChannelPause = bass->LoadFunction<BASS_ChannelPause>("BASS_ChannelPause");

		if (!bChannelPause(bass->stream))
		{
			BASS_ErrorGetCode bGetErrorCode = bass->LoadFunction<BASS_ErrorGetCode>("BASS_ErrorGetCode");
			std::cerr << "Error pausing stream" << std::endl;
			std::cerr << "Error code: " << bGetErrorCode() << std::endl;
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

		BASS_ChannelStop bChannelStop = bass->LoadFunction<BASS_ChannelStop>("BASS_ChannelStop");

		if (!bChannelStop(bass->stream))
		{
			BASS_ErrorGetCode bGetErrorCode = bass->LoadFunction<BASS_ErrorGetCode>("BASS_ErrorGetCode");
			std::cerr << "Error stopping stream" << std::endl;
			std::cerr << "Error code: " << bGetErrorCode() << std::endl;
			args.GetReturnValue().Set(
				String::NewFromUtf8(
					isolate,
					"Error occured when tried to stop a stream"
				)
			);
		}
		bass->stream = NULL;
	}
}