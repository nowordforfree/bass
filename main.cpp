#include <iostream>
#include "bass.h"

int main(int argc, char const *argv[])
{
  bool init = BASS_Init(-1, 44100, 0, 0, 0);
  if (!init)
  {
    std::cout << "BASS_Init error" << std::endl;
    return 1;
  }
  std::string filePath;
  std::cout << "Type path to file to play:" << std::endl;
  std::getline(std::cin, filePath);
  HSTREAM stream;
  stream = BASS_StreamCreateFile(FALSE, filePath.c_str(), 0, 0, 0);
  if (!stream)
  {
    std::cout << "Error creating stream" << std::endl;
    return 1;
  }
  BASS_ChannelPlay(stream, FALSE);
  // char stop;
  while (BASS_ChannelIsActive(stream) != BASS_ACTIVE_STOPPED)
  {
    std::cout << "Playing... Type \"q\" to stop" << std::endl;
    // std::cin >> stop;
  }
  BASS_ChannelStop(stream);
  BASS_Free();
  std::cout << "Executed without errors!" << std::endl ;
  return 0;
}