#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

#include "base/base.h"
#include "base/math.h"

#include "vertex.h"
#include "image.h"

enum class WrapOption
{
  REPEAT,
  MIRRORED_REPEAT,
  CLAMP_TO_EDGE,
  CLAMP_TO_BORDER,
};

enum class FilterOption
{
  LINEAR,
  NEAREST,
};

enum class TextureType
{
  // NOTE: would love to name this 2D but cant
  FLAT,
  CUBEMAP,
};

// NOTE: currently supports:
// - u8 rgba 2d textures
// - f32 depth component cubemap textures
class Texture
{
public:
  Texture(TextureType type, const uvec2& dimensions);
  Texture(const Image& image);
  Texture(
    const Image& image,
    WrapOption wrap_s,
    WrapOption wrap_t,
    FilterOption min_filter,
    FilterOption mag_filter
  );
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&& other);
  Texture& operator=(Texture&& other);
  ~Texture();

  void activate(u32 slot) const;

  [[nodiscard]] inline constexpr u32 handle() const
  {
    return m_id;
  }

private:
  void constructor_helper(
    void* data,
    WrapOption wrap_s,
    WrapOption wrap_t,
    WrapOption wrap_r,
    FilterOption min_filter,
    FilterOption mag_filter
  );

public:
  uvec2 dimensions;

private:
  TextureType m_type;
  u32 m_id;
};
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

class Shader
{
public:
  Shader(const std::filesystem::path& vs, const std::filesystem::path& fs);
  Shader(
    const std::filesystem::path& vs,
    const std::filesystem::path& fs,
    const std::filesystem::path& gs
  );
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  Shader(Shader&& other);
  Shader& operator=(Shader&& other);
  ~Shader();

  // TODO: array uniform support
  void use() const;
  void set(std::string_view name, i32 value);
  void set(std::string_view name, f32 value);
  void set(std::string_view name, const vec3& value);
  void set(std::string_view name, const mat4& value);
  void set(std::string_view name, TextureHandle handle);

  void reset_texture_slot();

private:
  u32 m_id{};
  u32 m_texture_slot{};
};
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

enum class RenderPrimitive
{
  TRIANGLES,
  LINE_STRIP,
};

// TODO: change this
struct RenderData;
class Mesh
{
public:
  Mesh(
    std::vector<Vertex>&& vertices,
    std::vector<u32>&& indices,
    std::vector<Submesh>&& submeshes,
    RenderPrimitive primitive,
    RenderData& render_data
  );
  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;
  Mesh(Mesh&& other);
  Mesh& operator=(Mesh&& other);
  ~Mesh();

  void use() const;

public:
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::vector<Submesh> submeshes;
  RenderPrimitive primitive;

private:
  u32 m_vbo;
  u32 m_ebo;
  u32 m_vao;
};
struct MeshHandle
{
  usize id;
};

class AssetManager
{
public:
  MeshHandle load_obj(const std::filesystem::path& path);
  TextureHandle load_texture(const std::filesystem::path& path);
  void clear();
  void bind_render_data(RenderData& render_data);

  inline constexpr static AssetManager& instance()
  {
    static AssetManager a{};
    return a;
  }

  inline constexpr TextureHandle set(Texture&& texture)
  {
    m_textures.emplace_back(std::move(texture));
    return {.id = m_textures.size() - 1};
  }
  inline constexpr MaterialHandle set(Material&& material)
  {
    m_materials.emplace_back(std::move(material));
    return {.id = m_materials.size() - 1};
  }
  inline constexpr MeshHandle set(Mesh&& mesh)
  {
    m_meshes.emplace_back(std::move(mesh));
    return {.id = m_meshes.size() - 1};
  }
  inline constexpr ShaderHandle set(Shader&& shader)
  {
    m_shaders.emplace_back(std::move(shader));
    return {.id = m_shaders.size() - 1};
  }

  inline constexpr Texture& get(TextureHandle handle)
  {
    return m_textures[handle.id];
  }
  inline constexpr Material& get(MaterialHandle handle)
  {
    return m_materials[handle.id];
  }
  inline constexpr Mesh& get(MeshHandle handle)
  {
    return m_meshes[handle.id];
  }
  inline constexpr Shader& get(ShaderHandle handle)
  {
    return m_shaders[handle.id];
  }

private:
  AssetManager() {}

  // TODO: change to a public load_materials function or something?
  void load_mtl_file(const std::filesystem::path& path);

private:
  RenderData* m_render_data;

  std::vector<Texture> m_textures;
  std::vector<Material> m_materials;
  std::vector<Mesh> m_meshes;
  std::vector<Shader> m_shaders;

  std::unordered_map<std::string, TextureHandle> m_texture_handles;
  std::unordered_map<std::string, MaterialHandle> m_material_handles;
};
