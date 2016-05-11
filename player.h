#ifndef PLAYER_H
#define PLAYER_H

#include <stack>
#include <node.h>
#include <node_object_wrap.h>
#include "bass.h"

#ifdef __cplusplus
extern "C" {
#endif
namespace bassplayer
{
	class BASSPlayer : public node::ObjectWrap
	{
	public:
		static void Init(v8::Local<v8::Object> exports);
	private:
		BASSPlayer();
		~BASSPlayer();

		static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Play(const v8::FunctionCallbackInfo<v8::Value> &args);
		static void Pause(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Stop(const v8::FunctionCallbackInfo<v8::Value>& args);
    static v8::Persistent<v8::Function> constructor;
    void *handl;
    HSTREAM stream;
    std::stack<HPLUGIN> plugins;
	};
}
#ifdef __cplusplus
}
#endif

#endif