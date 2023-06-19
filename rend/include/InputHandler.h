#pragma once
#include <SDL2/SDL.h>

class InputHandler {
  SDL_Event event;

public:
  bool poll();
};