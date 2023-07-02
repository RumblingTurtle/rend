#include <rend/InputHandler.h>

InputHandler::InputHandler() {
  for (int i = 0; i < KEY_CODE_COUNT; i++) {
    hold_time[i] = -HOLD_TIME;
  }
  reset();
}

void InputHandler::reset() {
  memset(key_pressed, 0, sizeof(key_pressed));
  memset(key_down, 0, sizeof(key_down));
  mouse_moved = false;
  m_dx = 0;
  m_dy = 0;
}

bool InputHandler::poll() {
  double dt = 1;
  if (first_poll) {
    prev_tick = Time::now();
    first_poll = false;
  } else {
    Time::TimePoint curr_tick = Time::now();
    dt = Time::time_difference<Time::Milliseconds>(curr_tick, prev_tick);
    prev_tick = curr_tick;
  }

  update_hold_time(dt);

  SDL_Event event;
  reset();
  while (SDL_PollEvent(&event)) {
    // The ESC key registers false positives for some reason
    if (event.type == SDL_QUIT || hold_time[KeyCode::ESC] > 0) {
      return false;
    }

    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      handle_key(event);
    }

    if (event.type == SDL_MOUSEMOTION) {
      handle_mouse_motion(event);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
      handle_mouse_button(event);
    }
  }

  return true;
}

KeyCode InputHandler::get_KeyCode(SDL_Event &event) {
  KeyCode KeyCode = KeyCode::KEY_CODE_COUNT;
  switch (event.key.keysym.sym) {
  case SDLK_ESCAPE:
    KeyCode = KeyCode::ESC;
    break;
  case SDLK_w:
    KeyCode = KeyCode::W;
    break;
  case SDLK_a:
    KeyCode = KeyCode::A;
    break;
  case SDLK_s:
    KeyCode = KeyCode::S;
    break;
  case SDLK_d:
    KeyCode = KeyCode::D;
    break;
  case SDLK_SPACE:
    KeyCode = KeyCode::SPACE;
    break;
  case SDLK_RETURN:
    KeyCode = KeyCode::ENTER;
    break;
  }

  return KeyCode;
}

void InputHandler::handle_key(SDL_Event &event) {
  KeyCode KeyCode = get_KeyCode(event);
  key_pressed[KeyCode] = true;
  key_down[KeyCode] = event.type == SDL_KEYDOWN;

  // Reset hold flag
  if (event.type == SDL_KEYUP) {
    hold_time[KeyCode] = -HOLD_TIME;
  }
}

void InputHandler::handle_mouse_motion(SDL_Event &event) {
  mouse_moved = true;
  m_dy = event.motion.yrel;
  m_dx = event.motion.xrel;
}

void InputHandler::handle_mouse_button(SDL_Event &event) {
  key_pressed[KeyCode::M_LEFT] = event.button.button == SDL_BUTTON_LEFT;
  key_pressed[KeyCode::M_RIGHT] = event.button.button == SDL_BUTTON_RIGHT;

  key_down[KeyCode::M_LEFT] = event.type == SDL_MOUSEBUTTONDOWN;
  key_down[KeyCode::M_RIGHT] = event.type == SDL_MOUSEBUTTONDOWN;
}

void InputHandler::update_hold_time(double dt) {
  for (int i = 0; i < KEY_CODE_COUNT; i++) {
    if (key_down[i]) { // First key press
      hold_time[i] += dt;
    } else {
      // Key is held until KEY_UP is registered
      if (hold_time[i] > -HOLD_TIME) {
        hold_time[i] += dt;
      }
    }
  }
}