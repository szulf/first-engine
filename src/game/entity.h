#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include <string>
#include <vector>
#include <expected>

#include "base/base.h"
#include "assets.h"

enum EntityType
{
  ENTITY_PLAYER,
  ENTITY_BLOCK,
  ENTITY_LIGHT_BULB,
  ENTITY_CONVEYOR,
  ENTITY_STORAGE,
};

std::expected<EntityType, std::string_view> entity_type_from_string(std::string_view str);

struct EntityPlayer
{
  static constexpr f32 MOVEMENT_SPEED = 8;
  static constexpr f32 ROTATION_SPEED = 3 * std::numbers::pi_v<f32>;
  static constexpr f32 MASS = 80;
  static constexpr f32 INTERACTION_RADIUS = 2;
  static constexpr std::string_view MESH_PATH = "assets/bean.obj";
};

struct EntityBlock
{
  static constexpr std::string_view MESH_PATH = "assets/cube.obj";
};

struct EntityLightBulb
{
  static constexpr vec3 ON_TINT = {1, 1, 1};
  static constexpr vec3 OFF_TINT = {0.1f, 0.1f, 0.1f};
  static constexpr vec3 ON_HOVER_COLOR = {0.3f, 1, 0.3f};
  static constexpr vec3 OFF_HOVER_COLOR = {0.4f, 0.4f, 0.4f};
  static constexpr f32 LIGHT_HEIGHT_OFFSET = -0.25f;
  static constexpr vec3 LIGHT_COLOR = {1, 1, 0.7f};
  static constexpr std::string_view MESH_PATH = "assets/light_bulb.obj";
  bool on{};
  bool hovered{};
};

struct EntityConveyor
{
  static constexpr std::string_view MESH_PATH = "assets/conveyor.obj";
};

struct EntityStorage
{
  static constexpr std::string_view MESH_PATH = "assets/storage.obj";
};

struct Entity
{
  EntityType type{};
  vec3 pos{};
  vec3 prev_pos{};
  f32 rotation{};
  f32 prev_rotation{};
  f32 target_rotation{};
  vec3 velocity{};
  vec2 bounding_box{};
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
