#include "assets.h"

#include <GL/glcorearb.h>
#include <filesystem>
#include <fstream>
#include <string>

#include "os/gl_functions.h"

#include "parser.h"
#include "renderer.h"

enum class ShaderType
{
  VERTEX,
  FRAGMENT,
  GEOMETRY,
};

static std::string_view shader_type_to_string(ShaderType type)
{
  switch (type)
  {
    case ShaderType::VERTEX:
      return "Vertex";
    case ShaderType::FRAGMENT:
      return "Fragment";
    case ShaderType::GEOMETRY:
      return "Geometry";
    default:
      return "Invalid";
  }
}

static GLenum gl_shader_type(ShaderType type)
{
  switch (type)
  {
    case ShaderType::VERTEX:
      return GL_VERTEX_SHADER;
    case ShaderType::FRAGMENT:
      return GL_FRAGMENT_SHADER;
    case ShaderType::GEOMETRY:
      return GL_GEOMETRY_SHADER;
  }
  ASSERT(false, "Invalid shader type.");
  return {};
}

static u32 shader_load(const std::filesystem::path& path, ShaderType shader_type)
{
  std::ifstream file_stream{path};
  ASSERT(!file_stream.fail(), "Failed to read file. Path: {}", path.string());
  std::stringstream ss{};
  ss << file_stream.rdbuf();
  auto file = ss.str();

  u32 shader = glCreateShader(gl_shader_type(shader_type));
  auto shader_src = file.c_str();
  glShaderSource(shader, 1, &shader_src, nullptr);
  glCompileShader(shader);
  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetShaderInfoLog(shader, 1024, &log_length, message);
    ASSERT(
      false,
      "{} shader failed with message:\n{}",
      shader_type_to_string(shader_type),
      message
    );
  }

  return shader;
}

static u32 shader_link(u32 vertex_shader, u32 fragment_shader, std::optional<u32> geometry_shader)
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  if (geometry_shader)
  {
    glAttachShader(program, *geometry_shader);
  }
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  if (geometry_shader)
  {
    glDeleteShader(*geometry_shader);
  }

  GLint program_linked;
  glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
  if (program_linked != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, &log_length, message);
    ASSERT(false, "Failed to link shaders with message:\n{}", message);
  }
  return program;
}

Shader::Shader(const std::filesystem::path& vs_path, const std::filesystem::path& fs_path)
{
  auto vs = shader_load(vs_path, ShaderType::VERTEX);
  auto fs = shader_load(fs_path, ShaderType::FRAGMENT);
  m_id = shader_link(vs, fs, std::nullopt);

  auto index = glGetUniformBlockIndex(m_id, "Camera");
  if (index != GL_INVALID_INDEX)
  {
    glUniformBlockBinding(m_id, index, RenderData::UBO_INDEX_CAMERA);
  }
  index = glGetUniformBlockIndex(m_id, "Lights");
  if (index != GL_INVALID_INDEX)
  {
    glUniformBlockBinding(m_id, index, RenderData::UBO_INDEX_LIGHTS);
  }
}

Shader::Shader(
  const std::filesystem::path& vs_path,
  const std::filesystem::path& fs_path,
  const std::filesystem::path& gs_path
)
{
  auto vs = shader_load(vs_path, ShaderType::VERTEX);
  auto fs = shader_load(fs_path, ShaderType::FRAGMENT);
  auto gs = shader_load(gs_path, ShaderType::GEOMETRY);
  m_id = shader_link(vs, fs, gs);

  auto index = glGetUniformBlockIndex(m_id, "Camera");
  if (index != GL_INVALID_INDEX)
  {
    glUniformBlockBinding(m_id, index, RenderData::UBO_INDEX_CAMERA);
  }
  index = glGetUniformBlockIndex(m_id, "Lights");
  if (index != GL_INVALID_INDEX)
  {
    glUniformBlockBinding(m_id, index, RenderData::UBO_INDEX_LIGHTS);
  }
}

Shader::Shader(Shader&& other) : m_id{other.m_id}
{
  other.m_id = 0;
}

Shader& Shader::operator=(Shader&& other)
{
  if (this == &other)
  {
    return *this;
  }
  glDeleteProgram(m_id);
  m_id = other.m_id;
  other.m_id = 0;
  return *this;
}

Shader::~Shader()
{
  glDeleteProgram(m_id);
}

void Shader::use() const
{
  glUseProgram(m_id);
}

void Shader::set(std::string_view name, i32 value)
{
  glUniform1i(glGetUniformLocation(m_id, name.data()), value);
}

void Shader::set(std::string_view name, f32 value)
{
  glUniform1f(glGetUniformLocation(m_id, name.data()), value);
}

void Shader::set(std::string_view name, const vec3& value)
{
  glUniform3f(glGetUniformLocation(m_id, name.data()), value.x, value.y, value.z);
}

void Shader::set(std::string_view name, const mat4& value)
{
  glUniformMatrix4fv(glGetUniformLocation(m_id, name.data()), 1, false, value.data[0].data());
}

void Shader::set(std::string_view name, TextureHandle handle)
{
  ASSERT(m_texture_slot < 16, "Reached max active textures.");
  AssetManager::instance().get(handle).activate(m_texture_slot);
  set(name, (i32) m_texture_slot);
  ++m_texture_slot;
}

void Shader::reset_texture_slot()
{
  m_texture_slot = 0;
}

Texture::Texture(TextureType type, const uvec2& dimensions) : m_type{type}
{
  constructor_helper(
    nullptr,
    dimensions,
    WrapOption::CLAMP_TO_EDGE,
    WrapOption::CLAMP_TO_EDGE,
    WrapOption::CLAMP_TO_EDGE,
    FilterOption::NEAREST,
    FilterOption::NEAREST
  );
}

Texture::Texture(const Image& image) : m_type{TextureType::FLAT}
{
  constructor_helper(
    image.data(),
    image.dimensions(),
    WrapOption::REPEAT,
    WrapOption::REPEAT,
    WrapOption::REPEAT,
    FilterOption::LINEAR,
    FilterOption::LINEAR
  );
}

Texture::Texture(
  const Image& image,
  WrapOption wrap_s,
  WrapOption wrap_t,
  FilterOption min_filter,
  FilterOption mag_filter
)
  : m_type{TextureType::FLAT}
{
  constructor_helper(
    image.data(),
    image.dimensions(),
    wrap_s,
    wrap_t,
    WrapOption::REPEAT,
    min_filter,
    mag_filter
  );
}

Texture::Texture(Texture&& other) : m_type{other.m_type}, m_id{other.m_id}
{
  other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other)
{
  if (this == &other)
  {
    return *this;
  }
  m_type = other.m_type;
  glDeleteTextures(1, &m_id);
  m_id = other.m_id;
  other.m_id = 0;
  return *this;
}

Texture::~Texture()
{
  glDeleteTextures(1, &m_id);
}

static GLenum gl_texture_type(TextureType type)
{
  switch (type)
  {
    case TextureType::FLAT:
      return GL_TEXTURE_2D;
    case TextureType::CUBEMAP:
      return GL_TEXTURE_CUBE_MAP;
  }
  ASSERT(false, "Invalid texture type.");
  return (GLenum) -1;
}

void Texture::activate(u32 slot) const
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(gl_texture_type(m_type), m_id);
}

static GLint gl_wrap_option(WrapOption option)
{
  switch (option)
  {
    case WrapOption::REPEAT:
      return GL_REPEAT;
    case WrapOption::MIRRORED_REPEAT:
      return GL_MIRRORED_REPEAT;
    case WrapOption::CLAMP_TO_EDGE:
      return GL_CLAMP_TO_EDGE;
    case WrapOption::CLAMP_TO_BORDER:
      return GL_CLAMP_TO_BORDER;
  }
}

static GLint gl_filter_option(FilterOption option)
{
  switch (option)
  {
    case FilterOption::LINEAR:
      return GL_LINEAR;
    case FilterOption::NEAREST:
      return GL_NEAREST;
  }
}

void Texture::constructor_helper(
  void* data,
  const uvec2& dimensions,
  WrapOption wrap_s,
  WrapOption wrap_t,
  WrapOption wrap_r,
  FilterOption min_filter,
  FilterOption mag_filter
)
{
  auto gl_type = gl_texture_type(m_type);
  glGenTextures(1, &m_id);
  glBindTexture(gl_type, m_id);

  glTexParameteri(gl_type, GL_TEXTURE_WRAP_S, gl_wrap_option(wrap_s));
  glTexParameteri(gl_type, GL_TEXTURE_WRAP_T, gl_wrap_option(wrap_t));
  glTexParameteri(gl_type, GL_TEXTURE_WRAP_R, gl_wrap_option(wrap_r));
  glTexParameteri(gl_type, GL_TEXTURE_MIN_FILTER, gl_filter_option(min_filter));
  glTexParameteri(gl_type, GL_TEXTURE_MAG_FILTER, gl_filter_option(mag_filter));

  switch (m_type)
  {
    case TextureType::FLAT:
    {
      glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        (GLsizei) dimensions.x,
        (GLsizei) dimensions.y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
      );
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    break;
    case TextureType::CUBEMAP:
    {
      for (u32 i = 0; i < 6; ++i)
      {
        glTexImage2D(
          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
          0,
          GL_DEPTH_COMPONENT,
          (GLsizei) dimensions.x,
          (GLsizei) dimensions.y,
          0,
          GL_DEPTH_COMPONENT,
          GL_FLOAT,
          data
        );
      }
    }
    break;
  }
}

Mesh::Mesh(
  std::vector<Vertex>&& vertices_,
  std::vector<u32>&& indices_,
  std::vector<Submesh>&& submeshes_,
  RenderPrimitive primitive_,
  RenderData& render_data_
)
  : vertices{vertices_}, indices{indices_}, submeshes{submeshes_}, primitive{primitive_}
{
  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  glGenBuffers(1, &m_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizei) (vertices.size() * sizeof(Vertex)),
    vertices.data(),
    GL_STATIC_DRAW
  );
  glGenBuffers(1, &m_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    (GLsizei) (indices.size() * sizeof(u32)),
    indices.data(),
    GL_STATIC_DRAW
  );

  {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      1,
      3,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Vertex),
      (void*) offsetof(Vertex, normal)
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
  }

  {
    glBindBuffer(GL_ARRAY_BUFFER, render_data_.instance_data_buffer);
    // NOTE: model matrix
    glVertexAttribPointer(
      3,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) offsetof(InstanceData, transform)
    );
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
      4,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) (offsetof(InstanceData, transform) + sizeof(vec4))
    );
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
      5,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) (offsetof(InstanceData, transform) + 2 * sizeof(vec4))
    );
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
      6,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) (offsetof(InstanceData, transform) + 3 * sizeof(vec4))
    );
    glEnableVertexAttribArray(6);
    // NOTE: entity tint
    glVertexAttribPointer(
      7,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) offsetof(InstanceData, tint)
    );
    glEnableVertexAttribArray(7);

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
  }

  glBindVertexArray(0);
}

Mesh::Mesh(Mesh&& other)
  : vertices{std::move(other.vertices)}, indices{std::move(other.indices)},
    submeshes{std::move(other.submeshes)}, primitive{other.primitive}, m_vbo{other.m_vbo},
    m_ebo{other.m_ebo}, m_vao{other.m_vao}
{
  other.m_vbo = 0;
  other.m_ebo = 0;
  other.m_vao = 0;
}

Mesh& Mesh::operator=(Mesh&& other)
{
  if (this == &other)
  {
    return *this;
  }
  vertices = std::move(other.vertices);
  indices = std::move(other.indices);
  submeshes = std::move(other.submeshes);
  primitive = other.primitive;
  glDeleteBuffers(1, &m_vbo);
  m_vbo = other.m_vbo;
  other.m_vbo = 0;
  glDeleteBuffers(1, &m_ebo);
  m_ebo = other.m_ebo;
  other.m_ebo = 0;
  glDeleteVertexArrays(1, &m_vao);
  m_vao = other.m_vao;
  other.m_vao = 0;
  return *this;
}

Mesh::~Mesh()
{
  glDeleteBuffers(1, &m_vbo);
  glDeleteBuffers(1, &m_ebo);
  glDeleteVertexArrays(1, &m_vao);
}

void Mesh::use() const
{
  glBindVertexArray(m_vao);
}

struct OBJContext
{
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::vector<Submesh> submeshes;
  std::vector<vec3> positions;
  std::vector<vec3> normals;
  std::vector<vec2> uvs;
  std::unordered_map<Vertex, std::size_t> vertex_cache{};
};

static vec3 obj_parse_vec3(parser::Pos& pos)
{
  vec3 out{};
  out.x = parser::number_f32(pos);
  out.y = parser::number_f32(pos);
  out.z = parser::number_f32(pos);
  return out;
}

void AssetManager::load_mtl_file(const std::filesystem::path& path)
{
  std::string mat_name{};
  Material mat{};
  bool parsing{};
  std::ifstream file{path};
  ASSERT(!file.fail(), "[MTL] File reading error. (path: {}).", path.string());
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty())
    {
      continue;
    }
    parser::Pos pos{.line = line};

    auto key = parser::word(pos);
    if (key == "newmtl")
    {
      if (parsing)
      {
        auto material_handle = set(std::move(mat));
        m_material_handles.insert_or_assign(mat_name, material_handle);
      }
      mat_name = parser::word(pos);
      if (m_material_handles.contains(mat_name))
      {
        parsing = false;
      }
      else
      {
        mat = {};
        parsing = true;
      }
    }
    else if (key == "Ka")
    {
      // NOTE: noop
    }
    else if (key == "map_Ka")
    {
      // NOTE: noop
    }
    else if (key == "Kd")
    {
      mat.diffuse_color = obj_parse_vec3(pos);
    }
    else if (key == "map_Kd")
    {
      auto filename = parser::word(pos);
      mat.diffuse_map = load_texture(path.parent_path() / filename);
    }
    else if (key == "Ks")
    {
      mat.specular_color = obj_parse_vec3(pos);
    }
    else if (key == "map_Ks")
    {
      // NOTE: noop
    }
    else if (key == "Ns")
    {
      mat.specular_exponent = parser::number_f32(pos);
    }
    else if (key == "map_Ns")
    {
      // NOTE: noop
    }
    else if (key == "#")
    {
      continue;
    }
    else if (key == "Ke")
    {
      // NOTE: noop
    }
    else if (key == "Ni")
    {
      // NOTE: noop
    }
    else if (key == "d")
    {
      // NOTE: noop
    }
    else if (key == "illum")
    {
      // NOTE: noop
    }
    else
    {
      ASSERT(false, "Invalid key found. ({}).", key);
    }
  }
  if (parsing)
  {
    auto material_handle = set(std::move(mat));
    m_material_handles.insert_or_assign(mat_name, material_handle);
  }
}

static void obj_parse_vertex(OBJContext& ctx, parser::Pos& pos)
{
  auto position_idx = parser::number_u32(pos);
  parser::expect_and_skip(pos, '/');
  auto uv_idx = parser::number_u32(pos);
  parser::expect_and_skip(pos, '/');
  auto normal_idx = parser::number_u32(pos);

  Vertex v{
    .pos = ctx.positions[position_idx - 1],
    .normal = ctx.normals[normal_idx - 1],
    .uv = ctx.uvs[uv_idx - 1],
  };

  if (ctx.vertex_cache.contains(v))
  {
    ctx.indices.push_back((u32) ctx.vertex_cache[v]);
  }
  else
  {
    auto idx = ctx.vertices.size();
    ctx.vertex_cache.insert_or_assign(v, idx);
    ctx.indices.push_back((u32) idx);
    ctx.vertices.push_back(v);
  }
  ++ctx.submeshes[ctx.submeshes.size() - 1].index_count;
}

MeshHandle AssetManager::load_obj(const std::filesystem::path& path)
{
  ASSERT(m_render_data, "Need to bind render data, before loading meshes.");
  OBJContext ctx{};
  std::ifstream file{path};
  ASSERT(!file.fail(), "File reading error. (path: {}).", path.string());
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    parser::Pos pos{.line = line};

    auto key = parser::word(pos);
    if (key == "mtllib")
    {
      auto mtl_filename = parser::word(pos);
      load_mtl_file(path.parent_path() / mtl_filename);
    }
    else if (key == "o")
    {
      ctx.submeshes.push_back({
        .index_offset = ctx.indices.size(),
      });
    }
    else if (key == "v")
    {
      auto position = obj_parse_vec3(pos);
      ctx.positions.push_back(position);
    }
    else if (key == "vn")
    {
      auto normal = obj_parse_vec3(pos);
      ctx.normals.push_back(normal);
    }
    else if (key == "vt")
    {
      vec2 uv{};
      uv.x = parser::number_f32(pos);
      uv.y = parser::number_f32(pos);
      ctx.uvs.push_back(uv);
    }
    else if (key == "s")
    {
      // NOTE: noop
    }
    else if (key == "usemtl")
    {
      auto name = parser::word(pos);
      ctx.submeshes[ctx.submeshes.size() - 1].material = m_material_handles[std::string{name}];
    }
    else if (key == "f")
    {
      obj_parse_vertex(ctx, pos);
      obj_parse_vertex(ctx, pos);
      obj_parse_vertex(ctx, pos);
    }
    else if (key == "#")
    {
      continue;
    }
    else
    {
      ASSERT(false, "Invalid key found. ({}).", key);
    }
  }

  return set(
    Mesh{
      std::move(ctx.vertices),
      std::move(ctx.indices),
      std::move(ctx.submeshes),
      RenderPrimitive::TRIANGLES,
      *m_render_data
    }
  );
}

TextureHandle AssetManager::load_texture(const std::filesystem::path& path)
{
  if (m_texture_handles.contains(path.string()))
  {
    return m_texture_handles[path.string()];
  }
  auto handle = set(Texture{{path}});
  m_texture_handles.insert_or_assign(path.string(), handle);
  return handle;
}

void AssetManager::clear()
{
  m_textures.clear();
  m_materials.clear();
  m_meshes.clear();

  m_texture_handles.clear();
  m_material_handles.clear();
}

void AssetManager::bind_render_data(RenderData& render_data)
{
  m_render_data = &render_data;
}
