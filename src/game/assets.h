#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

#include "base/base.h"
#include "base/math.h"

#include "vertex.h"
#include "image.h"

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

private:
  u32 m_id;
};
struct ShaderHandle
{
  usize id;
};

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

class Texture2D
{
public:
  Texture2D(const Image& image);
  Texture2D(
    const Image& image,
    WrapOption wrap_s,
    WrapOption wrap_t,
    FilterOption min_filter,
    FilterOption mag_filter
  );
  Texture2D(const Texture2D&) = delete;
  Texture2D& operator=(const Texture2D&) = delete;
  Texture2D(Texture2D&& other);
  Texture2D& operator=(Texture2D&& other);
  ~Texture2D();

  void activate(u32 slot) const;

private:
  void constructor_helper(
    const Image& image,
    WrapOption wrap_s,
    WrapOption wrap_t,
    FilterOption min_filter,
    FilterOption mag_filter
  );

private:
  u32 m_id;
};
struct Texture2DHandle
{
  usize id;
};

struct Material
{
  vec3 diffuse_color;
  vec3 specular_color;
  f32 specular_exponent;
  Texture2DHandle diffuse_map;
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
  Texture2DHandle load_texture(const std::filesystem::path& path);
  void clear();
  void bind_render_data(RenderData& render_data);

  inline constexpr static AssetManager& instance()
  {
    static AssetManager a{};
    return a;
  }

  inline constexpr Texture2DHandle set(Texture2D&& texture)
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

  inline constexpr Texture2D& get(Texture2DHandle handle)
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

  std::vector<Texture2D> m_textures;
  std::vector<Material> m_materials;
  std::vector<Mesh> m_meshes;
  std::vector<Shader> m_shaders;

  std::unordered_map<std::string, Texture2DHandle> m_texture_handles;
  std::unordered_map<std::string, MaterialHandle> m_material_handles;
};
