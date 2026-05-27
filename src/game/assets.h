#ifndef GAME_ASSETS_H
#define GAME_ASSETS_H

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

#include "base/base.h"
#include "base/math.h"
#include "vertex.h"
#include "image.h"

struct AssetStore;

enum WrapOption
{
  WRAP_OPTION_REPEAT,
  WRAP_OPTION_MIRRORED_REPEAT,
  WRAP_OPTION_CLAMP_TO_EDGE,
  WRAP_OPTION_CLAMP_TO_BORDER,
};

enum FilterOption
{
  FILTER_OPTION_LINEAR,
  FILTER_OPTION_NEAREST,
};

enum TextureType
{
  TEXTURE_2D,
  TEXTURE_CUBEMAP,
};

// NOTE: currently supports:
// - u8 rgba 2d textures
// - f32 depth component cubemap textures
struct Texture
{
  vec2 dimensions{};
  TextureType type{};
  u32 id{};
};

Texture texture_init(TextureType type, const vec2& dimensions);
Texture texture_from_image(
  const Image& img,
  WrapOption wrap_s = WRAP_OPTION_REPEAT,
  WrapOption wrap_t = WRAP_OPTION_REPEAT,
  FilterOption min_filter = FILTER_OPTION_NEAREST,
  FilterOption mag_filter = FILTER_OPTION_NEAREST
);
void texture_deinit(Texture& texture);
void texture_activate(const Texture& texture, u32 slot);

struct TextureHandle
{
  inline constexpr bool operator==(const TextureHandle& other) const
  {
    return id == other.id;
  }
  usize id;
};
template <>
struct std::hash<TextureHandle>
{
  std::size_t operator()(const TextureHandle& h) const noexcept
  {
    return h.id;
  }
};

struct Shader
{
  u32 id{};
  u32 texture_slot{};
};

std::expected<Shader, std::string_view> shader_from_file(
  const std::filesystem::path& vs,
  const std::filesystem::path& fs,
  const std::filesystem::path& gs = ""
);
void shader_deinit(Shader& shader);
// TODO: array uniform support
void shader_use(const Shader& shader);
void shader_set(Shader& shader, std::string_view name, i32 value);
void shader_set(Shader& shader, std::string_view name, f32 value);
void shader_set(Shader& shader, std::string_view name, const vec3& value);
void shader_set(Shader& shader, std::string_view name, const mat4& value);
void shader_set(Shader& shader, std::string_view name, TextureHandle handle, AssetStore& assets);
void shader_reset_texture_slot(Shader& shader);

struct ShaderHandle
{
  usize id;
};

struct Material
{
  vec3 diffuse_color;
  vec3 specular_color;
  f32 specular_exponent;
  TextureHandle diffuse_map;
};
struct MaterialHandle
{
  usize id;
};

struct Submesh
{
  usize index_offset;
  usize index_count;
  MaterialHandle material;
};

enum RenderPrimitive
{
  RENDER_PRIMITIVE_TRIANGLES,
  RENDER_PRIMITIVE_LINE_STRIP,
};

// TODO: change this
struct Render_Data;

struct Mesh
{
  std::vector<Vertex> vertices{};
  usize index_count{};
  std::vector<Submesh> submeshes{};
  RenderPrimitive primitive{};
  u32 vbo{};
  u32 ebo{};
  u32 vao{};
};

Mesh mesh_init(
  const std::vector<Vertex>& vertices,
  const std::vector<u32>& indices,
  const std::vector<Submesh>& submeshes,
  RenderPrimitive primitive,
  Render_Data& render_data
);
void mesh_deinit(Mesh& mesh);
void mesh_use(const Mesh& mesh);

struct MeshHandle
{
  usize id;
};

vec2 bounding_box_from_mesh(MeshHandle handle, AssetStore& assets);

struct AssetStore
{
  Render_Data* render_data{};
  std::vector<Texture> textures{};
  std::vector<Material> materials{};
  std::vector<Mesh> meshes{};
  std::vector<Shader> shaders{};
  std::unordered_map<std::string, TextureHandle> texture_handles{};
  std::unordered_map<std::string, MaterialHandle> material_handles{};
  std::unordered_map<std::string, MeshHandle> mesh_handles{};
};

MeshHandle load_obj(AssetStore& assets, const std::filesystem::path& path);
TextureHandle load_texture(AssetStore& assets, const std::filesystem::path& path);

inline void asset_store_bind_render_data(AssetStore& assets, Render_Data& render_data)
{
  assets.render_data = &render_data;
}

inline TextureHandle asset_set(AssetStore& assets, const Texture& texture)
{
  assets.textures.push_back(texture);
  return {.id = assets.textures.size() - 1};
}
inline MaterialHandle asset_set(AssetStore& assets, const Material& material)
{
  assets.materials.push_back(material);
  return {.id = assets.materials.size() - 1};
}
inline MeshHandle asset_set(AssetStore& assets, const Mesh& mesh)
{
  assets.meshes.push_back(mesh);
  return {.id = assets.meshes.size() - 1};
}
inline ShaderHandle asset_set(AssetStore& assets, const Shader& shader)
{
  assets.shaders.push_back(shader);
  return {.id = assets.shaders.size() - 1};
}

inline Texture& asset_get(AssetStore& assets, TextureHandle handle)
{
  return assets.textures[handle.id];
}
inline Material& asset_get(AssetStore& assets, MaterialHandle handle)
{
  return assets.materials[handle.id];
}
inline Mesh& asset_get(AssetStore& assets, MeshHandle handle)
{
  return assets.meshes[handle.id];
}
inline Shader& asset_get(AssetStore& assets, ShaderHandle handle)
{
  return assets.shaders[handle.id];
}

#endif
