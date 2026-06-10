#ifndef GAME_GAME_H
#define GAME_GAME_H

#include <array>

#include "base/base.h"
#include "os/os.h"
#include "sound.h"
#include "camera.h"
#include "entity.h"
#include "ui.h"

#define ACTIONS                                                                                    \
  X(ACTION_MOVE_FRONT, "move_front", OS_KEY_W)                                                     \
  X(ACTION_MOVE_BACK, "move_back", OS_KEY_S)                                                       \
  X(ACTION_MOVE_LEFT, "move_left", OS_KEY_A)                                                       \
  X(ACTION_MOVE_RIGHT, "move_right", OS_KEY_D)                                                     \
  X(ACTION_INTERACT, "interact", OS_KEY_E)                                                         \
  X(ACTION_OPEN_INVENTORY, "open_inventory", OS_KEY_TAB)                                           \
  X(ACTION_SLOT_1, "slot_1", OS_KEY_1)                                                             \
  X(ACTION_SLOT_2, "slot_2", OS_KEY_2)                                                             \
  X(ACTION_SLOT_3, "slot_3", OS_KEY_3)                                                             \
  X(ACTION_SLOT_4, "slot_4", OS_KEY_4)                                                             \
  X(ACTION_SAVE_SCENE, "save_scene", OS_KEY_K)                                                     \
  X(ACTION_LOAD_SCENE, "load_scene", OS_KEY_L)                                                     \
  X(ACTION_ROTATE_ENTITY_TO_PLACE, "rotate_entity_to_place", OS_KEY_R)                             \
  X(ACTION_CAMERA_MOVE_UP, "camera_move_up", OS_KEY_SPACE)                                         \
  X(ACTION_CAMERA_MOVE_DOWN, "camera_move_down", OS_KEY_LSHIFT)                                    \
  X(ACTION_TOGGLE_DEBUG_MENU, "toggle_debug_menu", OS_KEY_F1)                                      \
  X(ACTION_TOGGLE_NOCLIP, "toggle_noclip", OS_KEY_F2)

#define X(action, str, key) action,
enum Action
{
  ACTIONS ACTION_COUNT,
};
#undef X

#define X(action, str, key) map[action] = str;
static constexpr std::array<std::string_view, ACTION_COUNT> ACTION_TO_STRING = []()
{
  std::array<std::string_view, ACTION_COUNT> map{};
  ACTIONS
  return map;
}();
#undef X

using Keymap = std::array<OS_Key, ACTION_COUNT>;

#define X(action, str, key) k[action] = key;
static constexpr Keymap DEFAULT_KEYMAP = []() -> Keymap
{
  Keymap k{};
  ACTIONS
  return k;
}();
#undef X

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

void game_init(GameData& game, OS_Window& window, OS_Audio& audio);
void game_deinit(GameData& game);
void game_update_tick(GameData& game, f32 dt);
void game_update_frame(GameData& game, f32 t);
void game_render(GameData& game, f32 t);

inline OS_KeyState key_state_from_action(Action action, GameData& game)
{
  return game.window->input.keys[game.keymap[action]];
}

#endif
