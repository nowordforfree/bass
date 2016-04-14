#include <node.h>
#include "player.h"

namespace bassplayer
{
  using v8::Local;
  using v8::Object;

  void InitAll(Local<Object> exports)
  {
    BASSPlayer::Init(exports);
  }

  NODE_MODULE(bassplayer, InitAll)
}