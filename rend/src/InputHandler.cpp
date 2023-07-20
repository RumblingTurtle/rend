#include <rend/InputHandler.h>

namespace rend::input {
InputHandler::InputHandler() {
  memset(hold_time, 0, sizeof(hold_time));
  memset(key_pressed, 0, sizeof(key_pressed));
  memset(key_held, 0, sizeof(key_held));
  memset(key_released, 0, sizeof(key_released));

  int key_count;
  keyboard_state = SDL_GetKeyboardState(&key_count);
}

bool InputHandler::poll(float dt) {
  m_dx = 0;
  m_dy = 0;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      return false;
    }
  }

  handle_keyboard(dt);
  handle_mouse(dt);

  if (is_key_pressed(KeyCode::ESC)) {
    return false;
  }

  return true;
}

SDL_Scancode InputHandler::get_sdl_key_code(KeyCode code) {
  if (IH_TO_SDL_KEYMAP.find(code) == IH_TO_SDL_KEYMAP.end()) {
    return SDL_SCANCODE_UNKNOWN;
  }
  return IH_TO_SDL_KEYMAP[code];
}

KeyCode InputHandler::get_input_handler_key_code(SDL_Scancode code) {
  if (SDL_TO_IH_KEYMAP.find(code) == SDL_TO_IH_KEYMAP.end()) {
    return KEY_CODE_COUNT;
  }
  return SDL_TO_IH_KEYMAP[code];
}

void InputHandler::handle_keyboard(float dt) {
  for (int i = 0; i < KEY_CODE_COUNT; i++) {
    KeyCode ih_code = static_cast<KeyCode>(i);
    SDL_Scancode key_code = get_sdl_key_code(ih_code);

    bool key_down = keyboard_state[key_code];

    key_held[ih_code] = last_state[ih_code] && key_down;
    key_pressed[ih_code] = !last_state[ih_code] && key_down;
    key_released[ih_code] = last_state[ih_code] && key_down;
    last_state[ih_code] = key_down;

    if (key_held[ih_code]) {
      hold_time[ih_code] += dt;
    } else {
      hold_time[ih_code] = 0;
    }
  }
}

void InputHandler::handle_mouse(float dt) {

  SDL_GetRelativeMouseState(&m_dx, &m_dy);
  Uint32 button = SDL_GetMouseState(NULL, NULL);

  for (KeyCode key_code : {M_LEFT, M_RIGHT}) {
    bool key_down = key_code == M_LEFT ? button & SDL_BUTTON(SDL_BUTTON_LEFT)
                                       : button & SDL_BUTTON(SDL_BUTTON_RIGHT);
    key_pressed[key_code] = !last_state[key_code] && key_down;
    key_released[key_code] = last_state[key_code] && !key_down;
    key_held[key_code] = last_state[key_code] && key_down;
    last_state[key_code] = key_down;

    if (key_held[key_code]) {
      hold_time[key_code] += dt;
    } else {
      hold_time[key_code] = 0;
    }
  }
}

} // namespace rend::input