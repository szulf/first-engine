#pragma once

#include <string>
#include <vector>

#include "base/base.h"

#include "assets.h"

static constexpr f32 PLAYER_MOVEMENT_SPEED = 8.0f;
static constexpr f32 PLAYER_ROTATE_SPEED = 3 * std::numbers::pi_v<f32>;
static constexpr f32 PLAYER_MASS = 80.0f;

static constexpr vec3 LIGHT_BULB_ON_TINT = {1.0f, 1.0f, 1.0f};
static constexpr vec3 LIGHT_BULB_OFF_TINT = {0.1f, 0.1f, 0.1f};

vec2 bounding_box_from_mesh(MeshHandle mesh);

struct Entity
{
public:
  using Flags = u64;

  static constexpr Flags CONTROLLED_BY_PLAYER = 1 << 0;
  static constexpr Flags COLLIDABLE = 1 << 1;
  static constexpr Flags DYNAMIC_BOUNDING_BOX = 1 << 2;
  static constexpr Flags RENDERABLE = 1 << 3;
  static constexpr Flags INTERACTABLE = 1 << 4;
  static constexpr Flags EMITS_LIGHT = 1 << 5;

public:
  Entity() {}
  Entity(const std::filesystem::path& path);

  [[nodiscard]] inline constexpr bool controlled_by_player() const noexcept
  {
    return flags & CONTROLLED_BY_PLAYER;
  }
  [[nodiscard]] inline constexpr bool collidable() const noexcept
  {
    return flags & COLLIDABLE;
  }
  [[nodiscard]] inline constexpr bool dynamic_bounding_box() const noexcept
  {
    return flags & DYNAMIC_BOUNDING_BOX;
  }
  [[nodiscard]] inline constexpr bool renderable() const noexcept
  {
    return flags & RENDERABLE;
  }
  [[nodiscard]] inline constexpr bool interactable() const noexcept
  {
    return flags & INTERACTABLE;
  }
  [[nodiscard]] inline constexpr bool emits_light() const noexcept
  {
    return flags & EMITS_LIGHT;
  }

public:
  Flags flags{};

  vec3 pos;
  vec3 prev_pos{};
  vec3 rendered_pos{};

  f32 rotation{};
  f32 prev_rotation{};
  f32 rendered_rotation{};
  f32 target_rotation{};
  vec3 velocity{};

  vec2 bounding_box{};

  MeshHandle mesh{};

  f32 interactable_radius{};

  f32 light_height_offset{};
  vec3 light_color{};

  vec3 tint = {1.0f, 1.0f, 1.0f};

  // NOTE: read/write
  std::string name{};
  std::string mesh_path{};
};

bool entities_collide(const Entity& ea, const Entity& eb);

struct Scene
{
  vec3 ambient_color{};
  std::vector<Entity> entities{};
};

Scene load_scene(const std::filesystem::path& path);
