#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <vector>
#include <functional>

#include "base/base.h"
#include "assets.h"
#include "camera.h"

void render_init();
void render_create_framebuffer(TextureHandle texture);

enum Render_UBOIndices
{
  RENDER_UBO_INDEX_CAMERA = 0,
  RENDER_UBO_INDEX_LIGHTS = 1,
};

struct Render_Data
{
  Camera camera_2d{};
  u32 camera_ubo{};
  u32 lights_ubo{};
  u32 instance_data_buffer{};
  ShaderHandle default_shader{};
  ShaderHandle lighting_shader{};
  ShaderHandle quads_shader{};
  MeshHandle cube_wires{};
  MeshHandle ring{};
  MeshHandle line{};
  MeshHandle quad{};
  TextureHandle blank_texture{};
  std::unordered_map<TextureHandle, u32> framebuffers{};
};

#define RENDER_MAX_INSTANCES 10000
struct Render_InstanceData
{
  mat4 transform{};
  vec4 tint{};
  // NOTE: these are only used for 2D
  vec2 uv_scale = {1, 1};
  vec2 uv_offset = {0, 0};
  f32 corner_radius{};
};

struct Render_Cmd2D
{
  TextureHandle texture{};
  f32 z_idx{};
  Render_InstanceData instance_data{};
  std::optional<Rectangle> clip_rectangle{};
};

struct Render_Cmd3D
{
  MaterialHandle material{};
  MeshHandle mesh{};
  usize submesh_idx{};
  Render_InstanceData instance_data{};
};

#define RENDER_LIGHT_ATTENUATION_CONSTANT 1.0f
#define RENDER_LIGHT_ATTENUATION_LINEAR 0.22f
#define RENDER_LIGHT_ATTENUATION_QUADRATIC 0.2f
struct Render_Light
{
  vec3 pos{};
  vec3 color{};
};

enum Render_PassFlagsEnum
{
  PASS_OVERRIDED_FRAMEBUFFER = 1 << 0,
  PASS_OVERRIDED_SHADER = 1 << 1,
};
using Render_PassFlags = u32;

using Render_PassOnShaderBindCallback = std::function<void(Shader& shader)>;
struct Render_Pass
{
  f32 t{};
  Render_PassFlags flags{};
  const Camera* camera{};
  std::vector<Render_Cmd3D> cmds_3d{};
  std::vector<Render_Cmd2D> cmds_2d{};
  Render_PassOnShaderBindCallback on_shader_bind{};
  Render_Light light{};
  vec3 ambient_color = {1, 1, 1};
  u32 framebuffer{};
  ShaderHandle overidded_shader{};
};

void render_pass_finish(Render_Pass& pass);
void render_pass_render_to(Render_Pass& pass, TextureHandle texture);
void render_pass_override_shader(Render_Pass& pass, ShaderHandle shader);
void render_pass_on_shader_bind(Render_Pass& pass, Render_PassOnShaderBindCallback&& on_bind);
void render_pass_append(Render_Pass& pass, const Render_Cmd2D& cmd);
void render_pass_append(Render_Pass& pass, const std::vector<Render_Cmd2D>& cmd);
void render_pass_append(Render_Pass& pass, const Render_Cmd3D& cmd);
void render_pass_append(Render_Pass& pass, const std::vector<Render_Cmd3D>& cmd);
// NOTE: supports rendering only a single point light since that is what i need for the game,
// would not be too hard to implement more lights (will implement directional light later)
void render_pass_set_light(Render_Pass& pass, const vec3& pos, const vec3& color);

// NOTE: 2D
// TODO: commands with the same z-index are rendered wrong, disable depth test?
struct Render_Options2D
{
  f32 corner_radius = 0.0f;
  vec4 tint = {1, 1, 1, 1};
  std::optional<Rectangle> clip_rectangle = std::nullopt;
};
Render_Cmd2D render_quad(const vec3& pos, const vec2& size, const Render_Options2D& args = {});
Render_Cmd2D render_texture(
  TextureHandle texture,
  const vec3& pos,
  const vec2& size,
  const Render_Options2D& args = {}
);
Render_Cmd2D render_texture_part(
  TextureHandle texture,
  const vec3& pos,
  const vec2& size,
  const vec2& in_texture_pos,
  const vec2& in_texture_size,
  const Render_Options2D& args = {}
);

// NOTE: 3D
std::vector<Render_Cmd3D> render_mesh(
  MeshHandle handle,
  const vec3& pos,
  f32 rotation,
  const vec3& tint = {1.0f, 1.0f, 1.0f}
);
Render_Cmd3D render_cube_wires(const vec3& pos, const vec3& size, const vec3& color);
Render_Cmd3D render_ring(const vec3& pos, f32 radius, const vec3& color);
Render_Cmd3D render_line(const vec3& pos, f32 length, f32 rotation, const vec3& color);

#endif
