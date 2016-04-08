#include <iostream>
#include <stdlib.h>
#include "bass.h"
#ifdef _WIN32
  #ifdef _WIN64
    #define BASS_PATH "./win/x64/bass.dll"
    #pragma comment(lib, "./win/x64/bass.dll")
  #else
    #define BASS_PATH "./win/bass.dll"
    #pragma comment(lib, "./win/bass.dll")
  #endif
#elif __linux__
  #include <dlfcn.h>
  #if defined(__amd64__) || defined(__x86_64__)
    #define BASS_PATH "./linux/x64/libbass.so"
    #pragma comment(lib, "./linux/x64/libbass.so")
  #else
    #define BASS_PATH "./linux/libbass.so"
    #pragma comment(lib, "./linux/libbass.so")
  #endif
#endif

typedef void (*pf)();

int main(int argc, char const *argv[])
{
  void *bass;

  bass = dlopen (BASS_PATH, RTLD_LAZY);
  if (!bass) {
    std::cerr << dlerror() << std::endl;
    return 1;
  }

  typedef BOOL (*BASS_Init)(
    int device,
    DWORD freq,
    DWORD flags
  );
  typedef HSTREAM (*BASS_StreamCreateFile)(
    BOOL mem,
    void *file,
    QWORD offset,
    QWORD length,
    DWORD flags
  );
  typedef BOOL (*BASS_ChannelPlay)(
    DWORD handle,
    BOOL restart
  );
  typedef DWORD (*BASS_ChannelIsActive)(
    DWORD handle
  );
  typedef BOOL (*BASS_ChannelStop)(
    DWORD handle
  );
  typedef bool (*BASS_Free)();
  char *error;

  BASS_Init bInit = (BASS_Init) dlsym(bass, "BASS_Init");
  if ((error = dlerror()) != NULL)  {
    std::cerr << error << std::endl;
    return 1;
  }

  BASS_Free bFree = (BASS_Free) dlsym(bass, "BASS_Free");
  if ((error = dlerror()) != NULL)  {
    std::cerr << error << std::endl;
    return 1;
  }

  bool init = bInit(-1, 44100, 0);
  if (!init)
  {
    std::cout << "BASS_Init error" << std::endl;
    return 1;
  }
  std::string filePath;
  std::cout << "Type path to file to play:" << std::endl;
  std::getline(std::cin, filePath);

  BASS_StreamCreateFile bStreamFile = (BASS_StreamCreateFile) dlsym(bass, "BASS_StreamCreateFile");
  if ((error = dlerror()) != NULL)  {
    std::cerr << error << std::endl;
    return 1;
  }

  HSTREAM stream = bStreamFile(FALSE, (void*)filePath.c_str(), 0, 0, 0);
  if (!stream)
  {
    std::cout << "Error creating stream" << std::endl;
    return 1;
  }

  BASS_ChannelPlay bChannelPlay = (BASS_ChannelPlay) dlsym(bass, "BASS_ChannelPlay");
  if ((error = dlerror()) != NULL)  {
    std::cerr << error << std::endl;
    return 1;
  }

  BASS_ChannelIsActive bChannelActive = (BASS_ChannelIsActive) dlsym(bass, "BASS_ChannelIsActive");
  if ((error = dlerror()) != NULL)  {
    std::cerr << error << std::endl;
    return 1;
  }

  bChannelPlay(stream, FALSE);

  char stop;
  while (stop != 'q' && bChannelActive(stream) == BASS_ACTIVE_PLAYING)
  {
    std::cout << "Playing... Type \"q\" to stop" << std::endl;
    std::cin >> stop;
  }
  BASS_ChannelStop bChannelStop = (BASS_ChannelStop) dlsym(bass, "BASS_ChannelStop");
  if ((error = dlerror()) != NULL)  {
    std::cerr << error << std::endl;
    return 1;
  }
  bChannelStop(stream);
  bFree();
  dlclose(bass);
  std::cout << "Executed without errors!" << std::endl ;
  return 0;
}

// Rewrite to use Templating
bool LoadFunctions(std::string FuncNames[])
{
  if (FuncNames->length() == 0)
    return true;

  for (int i = 0; i < FuncNames->length(); i++)
  {

  }
}