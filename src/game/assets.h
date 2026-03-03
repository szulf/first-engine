#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

#include "base/base.h"
#include "base/math.h"

#include "vertex.h"
#include "image.h"

using ShaderHandle = usize;
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

using TextureHandle = usize;
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

using MaterialHandle = usize;
struct Material
{
  vec3 diffuse_color;
  vec3 specular_color;
  f32 specular_exponent;
  TextureHandle diffuse_map;
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
using MeshHandle = usize;
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

template <typename Handle, typename T>
class AssetType
{
public:
  Handle set(T&& t)
  {
    m_data.emplace_back(std::move(t));
    return m_data.size() - 1;
  }

  T& get(Handle handle)
  {
    return m_data[(usize) handle];
  }

  void clear()
  {
    m_data.clear();
  }

  [[nodiscard]] inline constexpr usize size() const
  {
    return m_data.size();
  }

private:
  std::vector<T> m_data{};
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

private:
  AssetManager() {}

  // TODO: change to a public load_materials function or something?
  void load_mtl_file(const std::filesystem::path& path);

public:
  AssetType<TextureHandle, Texture2D> textures;
  AssetType<MaterialHandle, Material> materials;
  AssetType<MeshHandle, Mesh> meshes;
  AssetType<ShaderHandle, Shader> shaders;

private:
  RenderData* m_render_data;

  std::unordered_map<std::string, TextureHandle> m_texture_handles;
  std::unordered_map<std::string, MaterialHandle> m_material_handles;
};
