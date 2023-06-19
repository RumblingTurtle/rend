#include <InputHandler.h>

bool InputHandler::poll() {
  bool poll_flag = SDL_PollEvent(&event);

  switch (event.key.keysym.sym) {
  case SDLK_ESCAPE:
    return false;

  default:
    break;
  }

  return true;
}