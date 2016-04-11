#include <iostream>
#include <fstream>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include "bass.h"
#ifdef _WIN32
  #include <windows.h>
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


/* Obtain a backtrace and print it to stdout. */
void print_trace (void)
{
  void *array[10];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);

  printf ("Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++)
     printf ("%s\n", strings[i]);

  free (strings);
}

class BASS {
  void *handl;
public:
  BASS();
  template <typename T> T LoadFunction (const char *funcName);
  ~BASS();
};

BASS::BASS ()
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
      std::cerr << dlerror() << std::endl;
      return;
    }
  #endif
}
template <typename T> T BASS::LoadFunction (const char *funcName)
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
    result = (T) dlsym(handl, funcName);
    if ((error = dlerror()) != NULL)
    {
      std::cerr << error << std::endl;
      return NULL;
    }
  #endif
  return result;
}
BASS::~BASS()
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

bool FileExists(const char *fname)
{
  return std::ifstream(fname) != NULL;
}

int main(int argc, char const *argv[])
{
  BASS *bass = new BASS();
  /* start methods signatures declaration */
  typedef BOOL (*BASS_Init)(
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
  typedef BOOL (*BASS_SetConfig)(
    DWORD option,
    DWORD value
  );
  typedef HSTREAM (*BASS_StreamCreateFile)(
    BOOL mem,
    void *file,
    QWORD offset,
    QWORD length,
    DWORD flags
  );
  typedef HSTREAM (*BASS_StreamCreateURL)(
    char *url,
    DWORD offset,
    DWORD flags,
    DOWNLOADPROC *proc,
    void *user
  );
  typedef HSAMPLE (*BASS_SampleLoad)(
    BOOL mem,
    void *file,
    QWORD offset,
    DWORD length,
    DWORD max,
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
  typedef int (*BASS_ErrorGetCode)();
  /* end methods signatures declaration */

  BASS_Init bInit = bass->LoadFunction<BASS_Init>("BASS_Init");
  BASS_SetConfig bSetConfig = bass->LoadFunction<BASS_SetConfig>("BASS_SetConfig");
  BASS_StreamCreateFile bStreamFile = bass->LoadFunction<BASS_StreamCreateFile>("BASS_StreamCreateFile");
  BASS_StreamCreateURL bStreamUrl = bass->LoadFunction<BASS_StreamCreateURL>("BASS_StreamCreateFile");
  BASS_SampleLoad bSampleLoad = bass->LoadFunction<BASS_SampleLoad>("BASS_SampleLoad");
  BASS_ChannelIsActive bChannelActive = bass->LoadFunction<BASS_ChannelIsActive>("BASS_ChannelIsActive");
  BASS_ChannelPlay bChannelPlay = bass->LoadFunction<BASS_ChannelPlay>("BASS_ChannelPlay");
  BASS_ChannelStop bChannelStop = bass->LoadFunction<BASS_ChannelStop>("BASS_ChannelStop");
  BASS_ErrorGetCode bGetErrorCode = bass->LoadFunction<BASS_ErrorGetCode>("BASS_ErrorGetCode");

  bool init = bInit(-1, 44100, 0, 0, 0);
  if (!init)
  {
    std::cout << "BASS_Init error" << std::endl;
    return 1;
  }
  bSetConfig(BASS_CONFIG_NET_PLAYLIST, 1);
  std::string str;
  std::cout << "Type path to file to play:" << std::endl;
  std::getline(std::cin, str);
  char * filePath = new char[str.length() + 1];
  std::copy(str.begin(), str.end(), filePath);
  filePath[str.size()] = '\0';
  HSTREAM stream;
  if (sizeof(filePath) > 0)
  {
    bool isStreamRemote = FileExists(filePath);
    if (isStreamRemote)
    {
      stream = bStreamFile(FALSE, (void*)filePath, 0, 0, 0);
      std::cout << "Stream local file using bStreamFile" << std::endl;
    }
    else
    {
      stream = bStreamUrl(filePath, 0, BASS_STREAM_BLOCK|BASS_STREAM_STATUS|BASS_STREAM_AUTOFREE, 0, 0);
      std::cout << "Stream remote file" << std::endl;
      print_trace();
    }
    if (!stream)
    {
      std::cout << "Error code: " << bGetErrorCode() << std::endl;
      std::cout << "Error creating stream" << std::endl;
      return 1;
    }
    bChannelPlay(stream, FALSE);
    char stop;
    while (stop != 'q')
    {
      std::cout << "Playing... Type \"q\" to stop" << std::endl;
      std::cin >> stop;
    }
  }
  else
  {
    std::cout << "Empty filePath parameter... exiting" << std::endl;
  }

  bChannelStop(stream);
  bass->~BASS();
  std::cout << "Executed without errors!" << std::endl ;
  return 0;
}