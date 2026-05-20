#pragma once

#include <string>
#include <vector>

#include "base/base.h"

#include "assets.h"

#define PLAYER_MOVEMENT_SPEED 8.0f
#define PLAYER_ROTATE_SPEED (3 * std::numbers::pi_v<f32>)
#define PLAYER_MASS 80.0f

#define LIGHT_BULB_ON_TINT vec3{1.0f, 1.0f, 1.0f}
#define LIGHT_BULB_OFF_TINT vec3{0.1f, 0.1f, 0.1f}

vec2 bounding_box_from_mesh(MeshHandle mesh);

enum EntityFlagsEnum
{
  ENTITY_CONTROLLED_BY_PLAYER = 1 << 0,
  ENTITY_COLLIDABLE = 1 << 1,
  ENTITY_TOGGLEABLE = 1 << 2,
  ENTITY_EMITS_LIGHT = 1 << 3,
  ENTITY_VISIBLE = 1 << 4,
  ENTITY_DYNAMIC_BOUNDING_BOX = 1 << 5,
};
using EntityFlags = u64;

struct Entity
{
  EntityFlags flags{};
  // NOTE: common
  vec3 pos{};
  vec3 prev_pos{};
  f32 rotation{};
  f32 prev_rotation{};
  f32 target_rotation{};
  vec3 velocity{};
  vec3 tint = {1.0f, 1.0f, 1.0f};
  // NOTE: collidable
  vec2 bounding_box{};
  // NOTE: toggleable
  bool toggle{};
  // NOTE: emits light
  f32 light_height_offset{};
  vec3 light_color{};
  // NOTE: visible
  MeshHandle mesh{};
  // NOTE: deprecated
  f32 interactable_radius{};
  // NOTE: serialization
  std::string name{};
  std::string mesh_path{};
};

std::expected<Entity, std::string_view> entity_from_file(const std::filesystem::path& path);
bool entities_collide(const Entity& ea, const Entity& eb);
vec3 entity_render_pos(const Entity& entity, f32 t);
f32 entity_render_rotation(const Entity& entity, f32 t);

struct Scene
{
  vec3 ambient_color{};
  std::vector<Entity> entities{};
};

std::expected<Scene, std::string_view> scene_from_file(const std::filesystem::path& path);
