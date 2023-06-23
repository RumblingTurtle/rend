#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <types.h>

class AudioPlayer {
  bool _valid;
  Mix_Music *music;

public:
  bool loop = false;
  Path audio_path;
  int format = MIX_INIT_MP3;

  AudioPlayer() {}

  bool load(Path path) {
    audio_path = path;
    if (SDL_Init(SDL_INIT_AUDIO)) {
      return false;
    };

    if (Mix_Init(MIX_INIT_MP3) != format) {
      std::cerr << "Could not initialize SDL mixer: " << Mix_GetError()
                << std::endl;
    }

    Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 640);
    music = Mix_LoadMUS(path.native().c_str());
    _valid = music != nullptr;
    return true;
  }

  void play() {
    if (_valid) {
      Mix_PlayMusic(music, loop ? -1 : 0);
    } else {
      std::cerr << "Could not play audio: " << Mix_GetError() << std::endl
                << audio_path << std::endl;
    }
  }

  ~AudioPlayer() { Mix_FreeMusic(music); }
};