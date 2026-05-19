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

#define SHADOW_MAP_DIMENSIONS uvec2{1024, 1024}
#define CHAR_SIZE vec2{9, 16}

struct GameData
{
  OS_Window* window{};
  Sound_System sound_system;
  UI_System ui_system{};

  Scene scene{};
  Keymap keymap{};

  Camera gameplay_camera{};
  Camera debug_camera{};
  Camera* used_camera{};

  TextureHandle shadow_map{};
  ShaderHandle shadow_depth_shader{};
  TextureHandle font_texture{};

  struct DebugOptions
  {
    struct Menu
    {
      bool shown{};
      bool open = true;
      bool drag{};
      vec3 pos = {100, 50, 0};
      vec2 drag_offset{};
    };
    Menu menu{};
    bool noclip{};
    bool display_bounding_boxes{};
  };
  DebugOptions debug{};
};

GameData game_init(OS_Window& window, OS_Audio& audio);
void game_deinit(GameData& game);
void game_update_tick(GameData& game, f32 dt);
void game_update_frame(GameData& game, f32 alpha);
void game_render(GameData& game);

inline OS_KeyState key_state_from_action(Action action, GameData& game)
{
  return game.window->input.keys[game.keymap.map[action]];
}

#endif
