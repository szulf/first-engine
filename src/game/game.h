#ifndef GAME_GAME_H
#define GAME_GAME_H

#include <array>

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

  ACTION_OPEN_INVENTORY,
  ACTION_SLOT_1,
  ACTION_SLOT_2,
  ACTION_SLOT_3,
  ACTION_SLOT_4,

  ACTION_SAVE_SCENE,
  ACTION_LOAD_SCENE,

  ACTION_ROTATE_ENTITY_TO_PLACE,

  ACTION_CAMERA_MOVE_UP,
  ACTION_CAMERA_MOVE_DOWN,
  ACTION_TOGGLE_DEBUG_MENU,
  ACTION_TOGGLE_NOCLIP,

  ACTION_COUNT,
};

struct Keymap
{
  OS_Key map[ACTION_COUNT]{};
};

static constexpr Keymap DEFAULT_KEYMAP = []() -> Keymap
{
  Keymap k{};
  k.map[ACTION_MOVE_FRONT] = OS_KEY_W;
  k.map[ACTION_MOVE_BACK] = OS_KEY_S;
  k.map[ACTION_MOVE_LEFT] = OS_KEY_A;
  k.map[ACTION_MOVE_RIGHT] = OS_KEY_D;
  k.map[ACTION_INTERACT] = OS_KEY_E;
  k.map[ACTION_OPEN_INVENTORY] = OS_KEY_TAB;
  k.map[ACTION_SLOT_1] = OS_KEY_1;
  k.map[ACTION_SLOT_2] = OS_KEY_2;
  k.map[ACTION_SLOT_3] = OS_KEY_3;
  k.map[ACTION_SLOT_4] = OS_KEY_4;
  k.map[ACTION_SAVE_SCENE] = OS_KEY_K;
  k.map[ACTION_LOAD_SCENE] = OS_KEY_L;
  k.map[ACTION_ROTATE_ENTITY_TO_PLACE] = OS_KEY_R;
  k.map[ACTION_CAMERA_MOVE_UP] = OS_KEY_SPACE;
  k.map[ACTION_CAMERA_MOVE_DOWN] = OS_KEY_LSHIFT;
  k.map[ACTION_TOGGLE_DEBUG_MENU] = OS_KEY_F1;
  k.map[ACTION_TOGGLE_NOCLIP] = OS_KEY_F2;
  return k;
}();

static constexpr vec2 SHADOW_MAP_DIMENSIONS = {1024, 1024};
static constexpr vec2 CHAR_SIZE = {9, 16};

struct GameData
{
  OS_Window* window{};
  Sound_System sound_system;
  UI_System ui_system{};
  AssetStore assets{};

  Scene scene{};
  Keymap keymap{};

  Camera gameplay_camera{};
  Camera debug_camera{};
  Camera* used_camera{};

  TextureHandle shadow_map{};
  ShaderHandle shadow_depth_shader{};
  TextureHandle font_texture{};

  std::array<TextureHandle, ITEM_TYPE_COUNT> item_icons{};

  vec3 mouse_tile_pos{};
  bool mouse_in_player_interaction_radius{};
  bool mouse_over_player_inventory{};

  struct DebugOptions
  {
    struct Menu
    {
      bool shown{};
      bool open{};
      bool drag{};
      vec3 pos{};
      vec2 drag_offset{};
    };
    Menu menu{.pos = {100, 50, 0}};
    Menu error_list{.pos = {400, 50, 0}};
    bool noclip{};
    bool display_bounding_boxes{};
  };
  DebugOptions debug{};
};

GameData game_init(OS_Window& window, OS_Audio& audio);
void game_deinit(GameData& game);
void game_update_tick(GameData& game, f32 dt);
void game_update_frame(GameData& game, f32 t);
void game_render(GameData& game, f32 t);

inline OS_KeyState key_state_from_action(Action action, GameData& game)
{
  return game.window->input.keys[game.keymap.map[action]];
}

#endif
