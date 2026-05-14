#ifndef GAME_H
#define GAME_H

#include "base/base.h"

#include "os/os.h"

#include "sound.h"
#include "camera.h"
#include "entity.h"
#include "ui.h"

enum Action
{
  ACTION_MOVE_FRONT,
  ACTION_MOVE_BACK,
  ACTION_MOVE_LEFT,
  ACTION_MOVE_RIGHT,
  ACTION_INTERACT,

  ACTION_CAMERA_MOVE_UP,
  ACTION_CAMERA_MOVE_DOWN,
  ACTION_TOGGLE_DEBUG_MENU,
  ACTION_TOGGLE_CAMERA_MODE,

  ACTION_COUNT,
};

struct Keymap
{
  OS_Key map[ACTION_COUNT]{};
};

class Game
{
public:
  Game(OS_Window& window, OS_Audio& audio);
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;
  Game(Game&&) = delete;
  Game& operator=(Game&&) = delete;

  void update_tick(f32 dt);
  void update_frame(f32 alpha);
  void render();

private:
  inline constexpr OS_KeyState& action_key(Action action)
  {
    return m_window.input.keys[m_keymap.map[action]];
  }

private:
  OS_Window& m_window;

  static constexpr uvec2 SHADOW_MAP_DIMENSIONS = {1024, 1024};
  TextureHandle shadow_map;
  ShaderHandle shadow_depth_shader;
  static constexpr vec2 CHAR_SIZE = {9, 16};
  TextureHandle font_texture;

  SoundSystem m_sound_system;
  UI_System ui_system{};

  Scene scene;
  Keymap m_keymap{};

  Camera m_gameplay_camera;
  Camera m_debug_camera;
  Camera* m_main_camera{};

  // TODO: can i somehow get rid of these flags?
  bool debug_menu_shown{};
  bool debug_menu_open{true};
  bool debug_menu_drag{};
  vec3 debug_menu_pos{100, 50, 0};
  vec2 debug_menu_drag_offset{};

  bool m_camera_mode{};
  bool m_display_bounding_boxes{};
};

#endif
