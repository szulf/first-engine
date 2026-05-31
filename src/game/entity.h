#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include <string>
#include <vector>
#include <expected>

#include "base/base.h"
#include "assets.h"
#include "items.h"

// TODO: im feeling another refactor of the entity system
// something that would combine the "flags/traits" and types

enum EntityType
{
  ENTITY_PLAYER,
  ENTITY_BLOCK,
  ENTITY_LIGHT_BULB,
  ENTITY_CONVEYOR,
  ENTITY_STORAGE,
  ENTITY_TYPE_COUNT,
};

// TODO: should these maybe just be arrays?
std::string_view entity_type_to_string(EntityType type);
std::expected<EntityType, std::string_view> entity_type_from_string(std::string_view str);

static constexpr std::array<EntityType, ITEM_TYPE_COUNT> ENTITY_TYPE_FROM_ITEM_TYPE = []()
{
  std::array<EntityType, ITEM_TYPE_COUNT> out{};
  out[ITEM_BLOCK] = ENTITY_BLOCK;
  out[ITEM_LIGHT_BULB] = ENTITY_LIGHT_BULB;
  out[ITEM_CONVEYOR] = ENTITY_CONVEYOR;
  out[ITEM_STORAGE] = ENTITY_STORAGE;
  return out;
}();

static constexpr std::array<std::string_view, ENTITY_TYPE_COUNT> ENTITY_MESH_PATH = []()
{
  std::array<std::string_view, ENTITY_TYPE_COUNT> out{};
  out[ENTITY_PLAYER] = "assets/bean.obj";
  out[ENTITY_BLOCK] = "assets/cube.obj";
  out[ENTITY_LIGHT_BULB] = "assets/light_bulb.obj";
  out[ENTITY_CONVEYOR] = "assets/conveyor.obj";
  out[ENTITY_STORAGE] = "assets/storage.obj";
  return out;
}();

extern std::array<vec2, ENTITY_TYPE_COUNT> ENTITY_BOUNDING_BOX;

static constexpr std::array<bool, ENTITY_TYPE_COUNT> ENTITY_ROTATABLE = []()
{
  std::array<bool, ENTITY_TYPE_COUNT> out{};
  out[ENTITY_PLAYER] = true;
  out[ENTITY_CONVEYOR] = true;
  out[ENTITY_STORAGE] = true;
  return out;
}();

struct EntityPlayer
{
  static constexpr f32 MOVEMENT_SPEED = 8;
  static constexpr f32 ROTATION_SPEED = 3 * std::numbers::pi_v<f32>;
  static constexpr f32 MASS = 80;
  static constexpr f32 INTERACTION_RADIUS = 2;
  f32 rotation{};
  f32 prev_rotation{};
  f32 target_rotation{};
  vec3 velocity{};
  static constexpr u8 HOTBAR_SLOT_COUNT = 4;
  // NOTE: slots [0; HOTBAR_SLOT_COUNT) are the hotbar
  std::array<ItemSlot, 16> inventory{};
  u8 selected_hotbar_slot{};
};

struct EntityBlock
{
};

struct EntityLightBulb
{
  static constexpr vec3 ON_TINT = {1, 1, 1};
  static constexpr vec3 OFF_TINT = {0.1f, 0.1f, 0.1f};
  static constexpr vec3 ON_HOVER_COLOR = {0.3f, 1, 0.3f};
  static constexpr vec3 OFF_HOVER_COLOR = {0.4f, 0.4f, 0.4f};
  static constexpr f32 LIGHT_HEIGHT_OFFSET = -0.25f;
  static constexpr vec3 LIGHT_COLOR = {1, 1, 0.7f};
  bool on{};
  bool hovered{};
};

struct EntityConveyor
{
  f32 rotation{};
};

struct EntityStorage
{
  f32 rotation{};
};

struct Entity
{
  EntityType type{};
  vec3 pos{};
  vec3 prev_pos{};
  vec3 tint = {1.0f, 1.0f, 1.0f};
  MeshHandle mesh{};
  union
  {
    EntityPlayer player;
    EntityBlock block;
    EntityLightBulb light_bulb;
    EntityConveyor conveyor;
    EntityStorage storage;
  };
};

Entity entity_new(EntityType type, AssetStore& assets);
bool entities_collide(const Entity& ea, const Entity& eb);
vec3 entity_render_pos(const Entity& entity, f32 t);
f32 entity_render_rotation(const Entity& entity, f32 t);

inline ItemSlot& player_selected_hotbar_slot(Entity& player)
{
  ASSERT(player.type == ENTITY_PLAYER, "entity needs to be of type ENTITY_PLAYER");
  return player.player.inventory[player.player.selected_hotbar_slot];
}
inline ItemSlot player_selected_hotbar_slot(const Entity& player)
{
  ASSERT(player.type == ENTITY_PLAYER, "entity needs to be of type ENTITY_PLAYER");
  return player.player.inventory[player.player.selected_hotbar_slot];
}
inline EntityType player_selected_hotbar_slot_entity_type(const Entity& player)
{
  return ENTITY_TYPE_FROM_ITEM_TYPE[player_selected_hotbar_slot(player).type];
}

struct Scene
{
  vec3 ambient_color{};
  std::vector<Entity> entities{};

  // TODO: make this an enum?
  // TODO: should this really be on the scene?
  // NOTE: represents rotation as k * pi/2,
  // where k is this variable and holds values in range [0; 3]
  u8 place_rotation{};
  std::vector<Entity> entity_place_queue{};
  std::vector<usize> entity_idx_remove_queue{};
};

std::expected<Scene, std::string_view>
load_scene(const std::filesystem::path& path, AssetStore& assets);
std::expected<void, std::string_view>
save_scene(const Scene& scene, const std::filesystem::path& path);

#endif
