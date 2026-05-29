#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include <string>
#include <vector>
#include <expected>

#include "base/base.h"
#include "assets.h"

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

std::string_view entity_type_to_string(EntityType type);
std::expected<EntityType, std::string_view> entity_type_from_string(std::string_view str);

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
  std::string name{};
  std::string mesh_path{};
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

struct Scene
{
  vec3 ambient_color{};
  std::vector<Entity> entities{};
};

std::expected<Scene, std::string_view>
scene_from_file(const std::filesystem::path& path, AssetStore& assets);

#endif
