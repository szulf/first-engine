#ifndef GAME_H
#define GAME_H

#include "base/base.h"
#include "base/enum_array.h"

#include "os/os.h"

#include "sound.h"
#include "camera.h"
#include "renderer.h"
#include "entity.h"
#include "ui.h"

enum class Action
{
  MOVE_FRONT,
  MOVE_BACK,
  MOVE_LEFT,
  MOVE_RIGHT,
  INTERACT,

  CAMERA_MOVE_UP,
  CAMERA_MOVE_DOWN,
  TOGGLE_DEBUG_MENU,
  TOGGLE_CAMERA_MODE,

  COUNT,
};

using Keymap = EnumArray<Action, os::Key>;

class Game
{
public:
  Game(os::Window& window, os::Audio& audio);
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;
  Game(Game&&) = delete;
  Game& operator=(Game&&) = delete;

  void update_tick(f32 dt);
  void update_frame(f32 alpha);
  void render();

private:
  inline constexpr os::KeyState& action_key(Action action)
  {
    return m_window.input().key(m_keymap[action]);
  }

private:
  os::Window& m_window;

  static constexpr uvec2 SHADOW_MAP_DIMENSIONS = {1024, 1024};
  TextureHandle shadow_map;
  ShaderHandle shadow_depth_shader;
  static constexpr uvec2 CHAR_SIZE = {9, 16};
  TextureHandle font_texture;

  std::vector<render::Cmd2D> ui_render_cmds{};

  SoundSystem m_sound_system;
  UI_System ui_system{};

  Scene scene;
  Keymap m_keymap{};

  Camera m_gameplay_camera;
  Camera m_debug_camera;
  Camera* m_main_camera{};

  // TODO: can i somehow get rid of these flags?
  bool debug_menu_shown{};
  // bool debug_menu_open{true};
  bool debug_menu_drag{};
  vec3 layout_pos{100, 50, 0};
  bool m_camera_mode{};
  bool m_display_bounding_boxes{};

  i32 test_scroll_value{};
  bool test_hover_value{};
};

#endif
