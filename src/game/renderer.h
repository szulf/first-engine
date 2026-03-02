#ifndef RENDERER_H
#define RENDERER_H

#include <vector>

#include "base/base.h"

#include "assets.h"
#include "camera.h"

// TODO: remove this
extern RenderData render_data;

// TODO: i dont like this
struct RenderData
{
  u32 camera_ubo{};
  u32 lights_ubo{};

  static constexpr uvec2 SHADOW_CUBEMAP_DIMENSIONS = {1024, 1024};
  // TODO: this should go through the TextureGPU class, but it doesnt handle cubemaps yet
  u32 shadow_cubemap{};
  u32 shadow_framebuffer_id{};

  u32 instance_data_buffer{};

  ShaderHandle default_shader;
  ShaderHandle lighting_shader;
  ShaderHandle shadow_depth_shader;
};

struct InstanceData
{
  static constexpr usize MAX = 10000;
  mat4 transform;
  vec3 tint;
};

struct RenderItem
{
  MaterialHandle material;
  MeshHandle mesh;
  usize submesh_idx;
  InstanceData instance_data;
};

struct Light
{
  vec3 pos;
  vec3 color;

  static constexpr f32 CONSTANT = 1.0f;
  static constexpr f32 LINEAR = 0.22f;
  static constexpr f32 QUADRATIC = 0.2f;
};

enum class RenderPassType
{
  FORWARD,
  POINT_SHADOW_MAP,
};

class RenderPass
{
public:
  void draw_mesh(
    MeshHandle handle,
    const vec3& pos,
    f32 rotation,
    const vec3& tint = {1.0f, 1.0f, 1.0f}
  );
  void draw_cube_wires(const vec3& pos, const vec3& size, const vec3& color);
  void draw_ring(const vec3& pos, f32 radius, const vec3& color);
  void draw_line(const vec3& pos, f32 length, f32 rotation, const vec3& color);

  // NOTE: supports rendering only a single point light since that is what i need for the game,
  // would not be too hard to implement more lights (will implement directional light later)
  void set_light(const vec3& pos, const vec3& color);
  // TODO: i feel like this method should take more arguments,
  // probably will fix itself when i will implement the directional light shadow map
  void use_shadow_map(const Camera& shadow_map_camera);

  void finish();

private:
  inline constexpr RenderPass(RenderPassType type, const Camera& camera, const vec3& ambient_color)
    : m_type{type}, m_camera{camera}, m_ambient_color{ambient_color}
  {
  }

private:
  // TODO: i dont like have a pass 'type'
  RenderPassType m_type{};
  const Camera& m_camera;
  std::vector<RenderItem> m_items{};
  Light m_light{};
  vec3 m_ambient_color{};
  const Camera* m_shadow_map_camera{};

  friend class Renderer;
};

class Renderer
{
public:
  Renderer();

  RenderPass
  begin_pass(RenderPassType type, const Camera& camera, const vec3& ambient_color = {1, 1, 1});
};

enum UBO_Index
{
  UBO_INDEX_CAMERA = 0,
  UBO_INDEX_LIGHTS,
};

struct STD140Camera
{
  mat4 proj_view;
  vec3 view_pos;
  float far_plane;
};

struct STD140Light
{
  vec4 pos;
  vec4 color;

  f32 constant;
  f32 linear;
  f32 quadratic;
  f32 _pad;
};

#endif
