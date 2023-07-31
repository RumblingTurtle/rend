#pragma once
#include <SDL2/SDL.h>
#include <rend/TimeUtils.h>
#include <unordered_map>

namespace rend::input {
enum KeyCode {
  M_LEFT,
  M_RIGHT,
  SPACE,
  ESC,
  ENTER,
  W,
  A,
  S,
  D,
  R,
  F,
  TAB,
  KEY_CODE_COUNT
};

static std::unordered_map<SDL_Scancode, KeyCode> SDL_TO_IH_KEYMAP = {
    {SDL_SCANCODE_SPACE, KeyCode::SPACE},  //
    {SDL_SCANCODE_ESCAPE, KeyCode::ESC},   //
    {SDL_SCANCODE_RETURN, KeyCode::ENTER}, //
    {SDL_SCANCODE_W, KeyCode::W},          //
    {SDL_SCANCODE_A, KeyCode::A},          //
    {SDL_SCANCODE_S, KeyCode::S},          //
    {SDL_SCANCODE_D, KeyCode::D},          //
    {SDL_SCANCODE_R, KeyCode::R},          //
    {SDL_SCANCODE_F, KeyCode::F},          //
    {SDL_SCANCODE_TAB, KeyCode::TAB},      //
};

static std::unordered_map<KeyCode, SDL_Scancode> IH_TO_SDL_KEYMAP = []() {
  std::unordered_map<KeyCode, SDL_Scancode> map;
  for (auto &pair : SDL_TO_IH_KEYMAP) {
    map[pair.second] = pair.first;
  }
  return map;
}();

struct InputHandler {
  bool key_pressed[KEY_CODE_COUNT];
  bool key_held[KEY_CODE_COUNT];
  bool key_released[KEY_CODE_COUNT];
  float hold_time[KEY_CODE_COUNT]; // in ms

  Uint32 last_state[KEY_CODE_COUNT];

  const Uint8 *keyboard_state;
  // Relative mouse movement
  int m_dx, m_dy;
  // Fill active and key_held arrays for this frame
  // Returns false on SDL_QUIT
  bool poll(float dt);

  KeyCode get_input_handler_key_code(SDL_Scancode code);
  SDL_Scancode get_sdl_key_code(KeyCode code);

  InputHandler();

  void handle_keyboard(float dt);
  void handle_mouse(float dt);

  bool is_key_held(KeyCode code) { return hold_time[code] > 0; }
  bool is_key_pressed(KeyCode code) { return key_pressed[code]; }
  bool is_key_released(KeyCode code) { return key_released[code]; }
};
} // namespace rend::input