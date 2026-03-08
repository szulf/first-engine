#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <functional>

#include "base/base.h"

#include "assets.h"
#include "camera.h"

struct RenderData
{
  Camera camera_2d{};

  static constexpr u32 UBO_INDEX_CAMERA = 0;
  u32 camera_ubo{};
  static constexpr u32 UBO_INDEX_LIGHTS = 1;
  u32 lights_ubo{};
  u32 instance_data_buffer{};

  ShaderHandle default_shader;
  ShaderHandle lighting_shader;

  MeshHandle cube_wires;
  MeshHandle ring;
  MeshHandle line;
  MeshHandle quad;

  TextureHandle blank_texture;

  std::unordered_map<TextureHandle, u32> framebuffers;
};

static constexpr usize MAX_INSTANCES = 10000;
struct InstanceData
{
  mat4 transform{};
  vec3 tint{};
  vec2 uv_scale{1, 1};
  vec2 uv_offset{0, 0};
};

struct RenderCmd3D
{
  MaterialHandle material{};
  MeshHandle mesh{};
  usize submesh_idx{};
  InstanceData instance_data{};
};

struct RenderCmd2D
{
  TextureHandle texture{};
  f32 z_idx{};
  InstanceData instance_data{};
};

struct Light
{
  vec3 pos;
  vec3 color;

  static constexpr f32 CONSTANT = 1.0f;
  static constexpr f32 LINEAR = 0.22f;
  static constexpr f32 QUADRATIC = 0.2f;
};

class RenderPass
{
  using OnShaderBindCallback = std::function<void(Shader& shader)>;

public:
  void render_to(TextureHandle texture);
  void override_shader(ShaderHandle shader);
  void on_shader_bind(OnShaderBindCallback&& on_bind);
  // TODO: when overriding shaders i should probably only bind it once
  // before iterating over all render commands
  void finish();

  // NOTE: 2D
  void draw_quad(const vec3& pos, const vec2& size, const vec3& color);
  void draw_texture(
    TextureHandle texture,
    const vec3& pos,
    const vec2& size,
    const vec3& tint = {1.0f, 1.0f, 1.0f}
  );
  // NOTE: texture (0, 0) is in the lower left corner
  void draw_texture_part(
    TextureHandle texture,
    const vec3& pos,
    const vec2& size,
    const vec2& in_texture_pos,
    const vec2& in_texture_size,
    const vec3& tint = {1.0f, 1.0f, 1.0f}
  );

  // NOTE: 3D
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

private:
  inline constexpr RenderPass(
    RenderData& render_data,
    const Camera& camera,
    const vec3& ambient_color
  )
    : m_render_data{render_data}, m_camera{camera}, m_ambient_color{ambient_color}
  {
  }

private:
  using Flags = u32;
  static constexpr Flags OVERRIDED_FRAMEBUFFER = 1 << 0;
  static constexpr Flags OVERRIDED_SHADER = 1 << 1;
  static constexpr Flags USES_SHADOW_MAP = 1 << 2;

  Flags m_flags{};
  RenderData& m_render_data;
  std::vector<RenderCmd3D> m_cmds_3d{};
  std::vector<RenderCmd2D> m_cmds_2d{};
  const Camera& m_camera;
  OnShaderBindCallback m_on_shader_bind{};

  Light m_light{};
  vec3 m_ambient_color{};

  u32 m_framebuffer{};

  ShaderHandle m_overidded_shader{};

  friend class Renderer;
};

class Renderer
{
public:
  Renderer();

  void create_framebuffer(TextureHandle texture);

  RenderPass begin_pass(const Camera& camera, const vec3& ambient_color = {1, 1, 1});

private:
  RenderData m_render_data;
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
