#pragma once
#include <SDL2/SDL.h>
#include <rend/TimeUtils.h>

enum KeyCode { M_LEFT, M_RIGHT, SPACE, ESC, ENTER, W, A, S, D, KEY_CODE_COUNT };

constexpr float HOLD_TIME = 50.0f; // in ms
struct InputHandler {
  bool mouse_moved;

  bool key_pressed[KEY_CODE_COUNT];
  bool key_down[KEY_CODE_COUNT];
  float hold_time[KEY_CODE_COUNT]; // in ms

  Time::TimePoint prev_tick;
  bool first_poll;
  // Relative mouse movement
  int m_dx, m_dy;
  // Fill active and key_down arrays for this frame
  // Returns false on SDL_QUIT
  bool poll();
  KeyCode get_KeyCode(SDL_Event &event);

  InputHandler();

  void reset();
  void handle_key(SDL_Event &event);
  void handle_mouse_button(SDL_Event &event);
  void handle_mouse_motion(SDL_Event &event);

  void update_hold_time(double dt);

  bool is_key_held(KeyCode code) { return hold_time[code] > 0; }
};