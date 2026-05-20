#include "assets.h"

#include <expected>
#include <filesystem>
#include <fstream>
#include <string>

#include "base/errors.h"
#include "os/gl_functions.h"
#include "parser.h"
#include "renderer.h"

static GLenum texture_type_to_gl_texture(TextureType type)
{
  switch (type)
  {
    case TEXTURE_2D:
      return GL_TEXTURE_2D;
    case TEXTURE_CUBEMAP:
      return GL_TEXTURE_CUBE_MAP;
  }
  ASSERT(false, "Invalid texture type");
}

static GLint wrap_option_to_gl_wrap(WrapOption option)
{
  switch (option)
  {
    case WRAP_OPTION_REPEAT:
      return GL_REPEAT;
    case WRAP_OPTION_MIRRORED_REPEAT:
      return GL_MIRRORED_REPEAT;
    case WRAP_OPTION_CLAMP_TO_EDGE:
      return GL_CLAMP_TO_EDGE;
    case WRAP_OPTION_CLAMP_TO_BORDER:
      return GL_CLAMP_TO_BORDER;
  }
  ASSERT(false, "Invalid wrap option");
}

static GLint filter_option_to_gl_filter(FilterOption option)
{
  switch (option)
  {
    case FILTER_OPTION_LINEAR:
      return GL_LINEAR;
    case FILTER_OPTION_NEAREST:
      return GL_NEAREST;
  }
  ASSERT(false, "Invalid filter option");
}

static void texture__init(
  Texture& texture,
  const void* data,
  WrapOption wrap_s,
  WrapOption wrap_t,
  WrapOption wrap_r,
  FilterOption min_filter,
  FilterOption mag_filter
)
{
  auto gl_type = texture_type_to_gl_texture(texture.type);
  glGenTextures(1, &texture.id);
  glBindTexture(gl_type, texture.id);

  glTexParameteri(gl_type, GL_TEXTURE_WRAP_S, wrap_option_to_gl_wrap(wrap_s));
  glTexParameteri(gl_type, GL_TEXTURE_WRAP_T, wrap_option_to_gl_wrap(wrap_t));
  glTexParameteri(gl_type, GL_TEXTURE_WRAP_R, wrap_option_to_gl_wrap(wrap_r));
  glTexParameteri(gl_type, GL_TEXTURE_MIN_FILTER, filter_option_to_gl_filter(min_filter));
  glTexParameteri(gl_type, GL_TEXTURE_MAG_FILTER, filter_option_to_gl_filter(mag_filter));

  switch (texture.type)
  {
    case TEXTURE_2D:
    {
      glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        (GLsizei) texture.dimensions.x,
        (GLsizei) texture.dimensions.y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
      );
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    break;
    case TEXTURE_CUBEMAP:
    {
      for (u32 i = 0; i < 6; ++i)
      {
        glTexImage2D(
          GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
          0,
          GL_DEPTH_COMPONENT,
          (GLsizei) texture.dimensions.x,
          (GLsizei) texture.dimensions.y,
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

Texture texture_init(TextureType type, const uvec2& dimensions)
{
  Texture texture{};
  texture.type = type;
  texture.dimensions = dimensions;
  texture__init(
    texture,
    nullptr,
    WRAP_OPTION_CLAMP_TO_EDGE,
    WRAP_OPTION_CLAMP_TO_EDGE,
    WRAP_OPTION_CLAMP_TO_EDGE,
    FILTER_OPTION_NEAREST,
    FILTER_OPTION_NEAREST
  );
  return texture;
}

Texture texture_from_image(
  const Image& img,
  WrapOption wrap_s,
  WrapOption wrap_t,
  FilterOption min_filter,
  FilterOption mag_filter
)
{
  Texture texture{};
  texture.type = TEXTURE_2D;
  texture.dimensions = img.dimensions;
  texture__init(
    texture,
    img.data.data(),
    wrap_s,
    wrap_t,
    WRAP_OPTION_CLAMP_TO_EDGE,
    min_filter,
    mag_filter
  );
  return texture;
}

void texture_deinit(Texture& texture)
{
  glDeleteTextures(1, &texture.id);
}

void texture_activate(const Texture& texture, u32 slot)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(texture_type_to_gl_texture(texture.type), texture.id);
}

enum ShaderType
{
  SHADER_TYPE_VERTEX,
  SHADER_TYPE_FRAGMENT,
  SHADER_TYPE_GEOMETRY,
};

static std::string_view shader_type_to_string(ShaderType type)
{
  switch (type)
  {
    case SHADER_TYPE_VERTEX:
      return "Vertex";
    case SHADER_TYPE_FRAGMENT:
      return "Fragment";
    case SHADER_TYPE_GEOMETRY:
      return "Geometry";
  }
  ASSERT(false, "Invalid shader type");
}

static GLenum shader_type_to_gl_shader(ShaderType type)
{
  switch (type)
  {
    case SHADER_TYPE_VERTEX:
      return GL_VERTEX_SHADER;
    case SHADER_TYPE_FRAGMENT:
      return GL_FRAGMENT_SHADER;
    case SHADER_TYPE_GEOMETRY:
      return GL_GEOMETRY_SHADER;
  }
  ASSERT(false, "Invalid shader type");
}

static std::expected<u32, std::string_view>
shader_load(const std::filesystem::path& path, ShaderType shader_type)
{
  std::ifstream file_stream{path};
  if (file_stream.fail())
  {
    return std::unexpected{"Failed to read shader file"};
  }
  std::stringstream ss{};
  ss << file_stream.rdbuf();
  auto file = ss.str();

  auto gl_shader_type = shader_type_to_gl_shader(shader_type);
  u32 shader = glCreateShader(gl_shader_type);
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
    // TODO: i hate not reporting the actual compilation message here
    return std::unexpected{"Failed to compile shader"};
  }

  return {shader};
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

std::expected<Shader, std::string_view> shader_from_file(
  const std::filesystem::path& vs_path,
  const std::filesystem::path& fs_path,
  const std::filesystem::path& gs_path
)
{
  Shader shader{};
  auto vs = shader_load(vs_path, SHADER_TYPE_VERTEX);
  if (!vs)
  {
    return std::unexpected{"Failed to load vertex shader"};
  }
  auto fs = shader_load(fs_path, SHADER_TYPE_FRAGMENT);
  if (!fs)
  {
    return std::unexpected{"Failed to load fragment shader"};
  }
  if (gs_path.empty())
  {
    shader.id = shader_link(*vs, *fs, std::nullopt);
  }
  else
  {
    auto gs = shader_load(gs_path, SHADER_TYPE_GEOMETRY);
    if (!gs)
    {
      return std::unexpected{"Failed to load geometry shader"};
    }
    shader.id = shader_link(*vs, *fs, *gs);
  }

  auto index = glGetUniformBlockIndex(shader.id, "Camera");
  if (index != GL_INVALID_INDEX)
  {
    glUniformBlockBinding(shader.id, index, RENDER_UBO_INDEX_CAMERA);
  }
  index = glGetUniformBlockIndex(shader.id, "Lights");
  if (index != GL_INVALID_INDEX)
  {
    glUniformBlockBinding(shader.id, index, RENDER_UBO_INDEX_LIGHTS);
  }
  return {shader};
}

void shader_deinit(Shader& shader)
{
  glDeleteProgram(shader.id);
}

void shader_use(const Shader& shader)
{
  glUseProgram(shader.id);
}

void shader_set(Shader& shader, std::string_view name, i32 value)
{
  glUniform1i(glGetUniformLocation(shader.id, name.data()), value);
}

void shader_set(Shader& shader, std::string_view name, f32 value)
{
  glUniform1f(glGetUniformLocation(shader.id, name.data()), value);
}

void shader_set(Shader& shader, std::string_view name, const vec3& value)
{
  glUniform3f(glGetUniformLocation(shader.id, name.data()), value.x, value.y, value.z);
}

void shader_set(Shader& shader, std::string_view name, const mat4& value)
{
  glUniformMatrix4fv(glGetUniformLocation(shader.id, name.data()), 1, false, value.data[0]);
}

void shader_set(Shader& shader, std::string_view name, TextureHandle handle)
{
  ASSERT(shader.texture_slot < 16, "Reached max active textures.");
  texture_activate(asset_get(g_assets, handle), shader.texture_slot);
  shader_set(shader, name, (i32) shader.texture_slot);
  ++shader.texture_slot;
}

void shader_reset_texture_slot(Shader& shader)
{
  shader.texture_slot = 0;
}

Mesh mesh_init(
  const std::vector<Vertex>& vertices,
  const std::vector<u32>& indices,
  const std::vector<Submesh>& submeshes,
  RenderPrimitive primitive,
  Render_Data& render_data
)
{
  Mesh mesh{};
  mesh.vertices = vertices;
  mesh.index_count = indices.size();
  mesh.submeshes = submeshes;
  mesh.primitive = primitive;

  glGenVertexArrays(1, &mesh.vao);
  glBindVertexArray(mesh.vao);

  glGenBuffers(1, &mesh.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizei) (vertices.size() * sizeof(Vertex)),
    vertices.data(),
    GL_STATIC_DRAW
  );
  glGenBuffers(1, &mesh.ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
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
    glBindBuffer(GL_ARRAY_BUFFER, render_data.instance_data_buffer);
    // NOTE: model matrix
    glVertexAttribPointer(
      3,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Render_InstanceData),
      (void*) offsetof(Render_InstanceData, transform)
    );
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
      4,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Render_InstanceData),
      (void*) (offsetof(Render_InstanceData, transform) + sizeof(vec4))
    );
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
      5,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Render_InstanceData),
      (void*) (offsetof(Render_InstanceData, transform) + 2 * sizeof(vec4))
    );
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
      6,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Render_InstanceData),
      (void*) (offsetof(Render_InstanceData, transform) + 3 * sizeof(vec4))
    );
    glEnableVertexAttribArray(6);
    // NOTE: entity tint
    glVertexAttribPointer(
      7,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Render_InstanceData),
      (void*) offsetof(Render_InstanceData, tint)
    );
    glEnableVertexAttribArray(7);
    // NOTE: for rendering parts of a texture
    glVertexAttribPointer(
      8,
      2,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Render_InstanceData),
      (void*) offsetof(Render_InstanceData, uv_scale)
    );
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(
      9,
      2,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Render_InstanceData),
      (void*) offsetof(Render_InstanceData, uv_offset)
    );
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(
      10,
      1,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Render_InstanceData),
      (void*) offsetof(Render_InstanceData, corner_radius)
    );
    glEnableVertexAttribArray(10);

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);
    glVertexAttribDivisor(9, 1);
    glVertexAttribDivisor(10, 1);
  }

  glBindVertexArray(0);
  return mesh;
}

void mesh_deinit(Mesh& mesh)
{
  glDeleteBuffers(1, &mesh.vbo);
  glDeleteBuffers(1, &mesh.ebo);
  glDeleteVertexArrays(1, &mesh.vao);
}

void mesh_use(const Mesh& mesh)
{
  glBindVertexArray(mesh.vao);
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

static vec3 obj_parse_vec3(Parser_Pos& pos)
{
  vec3 out{};
  out.x = parser_number_f32(pos);
  out.y = parser_number_f32(pos);
  out.z = parser_number_f32(pos);
  return out;
}

static void load_mtl_file(AssetStore& assets, const std::filesystem::path& path)
{
  std::string mat_name{};
  Material mat{};
  bool parsing{};
  std::ifstream file{path};
  if (file.fail())
  {
    REPORT_ERROR("Failed to open mtl file");
    return;
  }
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty())
    {
      continue;
    }
    Parser_Pos pos{.line = line};

    auto key = parser_word(pos);
    if (key == "newmtl")
    {
      if (parsing)
      {
        auto material_handle = asset_set(assets, mat);
        assets.material_handles.insert_or_assign(mat_name, material_handle);
      }
      mat_name = parser_word(pos);
      if (assets.material_handles.contains(mat_name))
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
      auto filename = parser_word(pos);
      mat.diffuse_map = load_texture(g_assets, path.parent_path() / filename);
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
      mat.specular_exponent = parser_number_f32(pos);
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
      REPORT_ERROR("Invalid mtl key");
      continue;
    }
  }
  if (parsing)
  {
    auto material_handle = asset_set(g_assets, mat);
    assets.material_handles.insert_or_assign(mat_name, material_handle);
  }
}

static void obj_parse_vertex(OBJContext& ctx, Parser_Pos& pos)
{
  auto position_idx = parser_number_u32(pos);
  parser_expect_and_skip(pos, '/');
  auto uv_idx = parser_number_u32(pos);
  parser_expect_and_skip(pos, '/');
  auto normal_idx = parser_number_u32(pos);

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

MeshHandle load_obj(AssetStore& assets, const std::filesystem::path& path)
{
  ASSERT(assets.render_data, "Need to bind render data, before loading meshes.");
  OBJContext ctx{};
  std::ifstream file{path};
  if (file.fail())
  {
    REPORT_ERROR("Failed to open obj file");
    return assets.render_data->quad;
  }
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    Parser_Pos pos{.line = line};

    auto key = parser_word(pos);
    if (key == "mtllib")
    {
      auto mtl_filename = parser_word(pos);
      load_mtl_file(assets, path.parent_path() / mtl_filename);
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
      uv.x = parser_number_f32(pos);
      uv.y = parser_number_f32(pos);
      ctx.uvs.push_back(uv);
    }
    else if (key == "s")
    {
      // NOTE: noop
    }
    else if (key == "usemtl")
    {
      auto name = parser_word(pos);
      ctx.submeshes[ctx.submeshes.size() - 1].material = assets.material_handles[std::string{name}];
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
      REPORT_ERROR("Invalid obj key");
      return assets.render_data->quad;
    }
  }

  return asset_set(
    g_assets,
    mesh_init(
      ctx.vertices,
      ctx.indices,
      ctx.submeshes,
      RENDER_PRIMITIVE_TRIANGLES,
      *assets.render_data
    )
  );
}

TextureHandle load_texture(AssetStore& assets, const std::filesystem::path& path)
{
  if (assets.texture_handles.contains(path.string()))
  {
    return assets.texture_handles[path.string()];
  }
  auto img = image_from_file(path);
  if (!img)
  {
    REPORT_ERROR(img.error());
    return assets.render_data->blank_texture;
  }
  auto handle = asset_set(assets, texture_from_image(*img));
  assets.texture_handles.insert_or_assign(path.string(), handle);
  return handle;
}

AssetStore g_assets = {};
