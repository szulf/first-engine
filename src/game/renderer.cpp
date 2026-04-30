#include "renderer.h"

#include <algorithm>

#include "base/math.h"
#include "game/assets.h"
#include "game/camera.h"
#include "os/gl_functions.h"

namespace render
{

static Data render_data{};

void Pass::render_to(TextureHandle texture)
{
  ASSERT(
    render_data.framebuffers.contains(texture),
    "No framebuffer associated with this texture."
  );
  m_flags |= OVERRIDED_FRAMEBUFFER;
  m_framebuffer = render_data.framebuffers[texture];
}

void Pass::override_shader(ShaderHandle shader)
{
  m_flags |= OVERRIDED_SHADER;
  m_overidded_shader = shader;
}

void Pass::on_shader_bind(OnShaderBindCallback&& on_bind)
{
  m_on_shader_bind = std::move(on_bind);
}

static GLenum gl_primitive(RenderPrimitive primitive)
{
  switch (primitive)
  {
    case RenderPrimitive::TRIANGLES:
    {
      return GL_TRIANGLES;
    }
    break;
    case RenderPrimitive::LINE_STRIP:
    {
      return GL_LINE_STRIP;
    }
    break;
  }
}

void Pass::finish()
{
  auto& assets = AssetManager::instance();

  // NOTE: 3D
  std::ranges::sort(
    m_cmds_3d,
    [](const Cmd3D& a, const Cmd3D& b) -> bool
    {
      if (a.material.id != b.material.id)
      {
        return a.material.id < b.material.id;
      }
      if (a.mesh.id != b.mesh.id)
      {
        return a.mesh.id > b.mesh.id;
      }
      return a.submesh_idx > b.submesh_idx;
    }
  );

  if (m_flags & OVERRIDED_FRAMEBUFFER)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  }
  else
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  glDisable(GL_SCISSOR_TEST);
  glViewport(0, 0, (GLsizei) m_camera.viewport().x, (GLsizei) m_camera.viewport().y);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  STD140Camera camera_std140 = {};
  camera_std140.view_pos = m_camera.pos();
  camera_std140.proj_view = m_camera.projection() * m_camera.look_at();
  camera_std140.far_plane = m_camera.far_plane();
  glBindBuffer(GL_UNIFORM_BUFFER, render_data.camera_ubo);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera_std140), &camera_std140);

  STD140Light light_std140 = {};
  light_std140.pos = {m_light.pos.x, m_light.pos.y, m_light.pos.z, 1.0f};
  light_std140.color = {m_light.color.x, m_light.color.y, m_light.color.z, 1.0f};
  light_std140.constant = Light::CONSTANT;
  light_std140.linear = Light::LINEAR;
  light_std140.quadratic = Light::QUADRATIC;
  glBindBuffer(GL_UNIFORM_BUFFER, render_data.lights_ubo);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(STD140Light), &light_std140);

  for (usize cmd_idx = 0; cmd_idx < m_cmds_3d.size();)
  {
    const auto& cmd = m_cmds_3d[cmd_idx];
    const auto& mesh = assets.get(cmd.mesh);
    const auto& submesh = mesh.submeshes[cmd.submesh_idx];
    const auto& material = assets.get(cmd.material);

    usize batch_idx = cmd_idx + 1;
    while ((batch_idx < m_cmds_3d.size() && batch_idx - cmd_idx < MAX_INSTANCES) &&
           cmd.mesh.id == m_cmds_3d[batch_idx].mesh.id &&
           cmd.submesh_idx == m_cmds_3d[batch_idx].submesh_idx &&
           cmd.material.id == m_cmds_3d[batch_idx].material.id)
    {
      ++batch_idx;
    }
    const auto batch_size = batch_idx - cmd_idx;

    std::vector<InstanceData> instance_data{};
    instance_data.reserve(batch_size);
    for (usize i = cmd_idx; i < batch_idx; ++i)
    {
      instance_data.push_back(m_cmds_3d[i].instance_data);
    }

    ShaderHandle handle = render_data.default_shader;
    if (m_flags & OVERRIDED_SHADER)
    {
      handle = m_overidded_shader;
    }
    else if (material.specular_exponent != 0.0f)
    {
      handle = render_data.lighting_shader;
    }
    auto& shader = assets.get(handle);

    shader.use();
    if (m_on_shader_bind)
    {
      m_on_shader_bind(shader);
    }

    shader.set("material.ambient", m_ambient_color);
    shader.set("material.diffuse", material.diffuse_color);
    shader.set("material.specular", material.specular_color);
    shader.set("material.specular_exponent", material.specular_exponent);
    shader.set("material.diffuse_map", material.diffuse_map);

    glBindBuffer(GL_ARRAY_BUFFER, render_data.instance_data_buffer);
    glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      (GLsizeiptr) (instance_data.size() * sizeof(InstanceData)),
      instance_data.data()
    );

    mesh.use();

    glDrawElementsInstanced(
      gl_primitive(mesh.primitive),
      (GLsizei) submesh.index_count,
      GL_UNSIGNED_INT,
      (void*) (submesh.index_offset * sizeof(u32)),
      (GLsizei) batch_size
    );

    cmd_idx += batch_size;
    shader.reset_texture_slot();
  }

  // NOTE: 2D
  std::ranges::sort(
    m_cmds_2d,
    [](const Cmd2D& a, const Cmd2D& b)
    {
      if (!f32_equal(a.z_idx, b.z_idx))
      {
        return a.z_idx < b.z_idx;
      }
      return a.texture.id > b.texture.id;
    }
  );

  glEnable(GL_SCISSOR_TEST);

  camera_std140 = {};
  render_data.camera_2d.update_viewport(m_camera.viewport());
  camera_std140.view_pos = render_data.camera_2d.pos();
  camera_std140.proj_view = render_data.camera_2d.projection() * render_data.camera_2d.look_at();
  camera_std140.far_plane = render_data.camera_2d.far_plane();
  glBindBuffer(GL_UNIFORM_BUFFER, render_data.camera_ubo);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera_std140), &camera_std140);

  const auto& quad = assets.get(render_data.quad);
  for (usize cmd_idx = 0; cmd_idx < m_cmds_2d.size();)
  {
    const auto& cmd = m_cmds_2d[cmd_idx];

    usize batch_idx = cmd_idx + 1;
    while ((batch_idx < m_cmds_2d.size() && batch_idx - cmd_idx < MAX_INSTANCES) &&
           cmd.texture == m_cmds_2d[batch_idx].texture &&
           cmd.clip_rectangle == m_cmds_2d[batch_idx].clip_rectangle)
    {
      ++batch_idx;
    }
    const auto batch_size = batch_idx - cmd_idx;

    std::vector<InstanceData> instance_data{};
    instance_data.reserve(batch_size);
    for (usize i = cmd_idx; i < batch_idx; ++i)
    {
      instance_data.push_back(m_cmds_2d[i].instance_data);
    }

    auto& shader = assets.get(render_data.quads_shader);
    shader.use();
    shader.set("diffuse_map", cmd.texture);

    glBindBuffer(GL_ARRAY_BUFFER, render_data.instance_data_buffer);
    glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      (GLsizeiptr) (instance_data.size() * sizeof(InstanceData)),
      instance_data.data()
    );

    quad.use();

    if (cmd.clip_rectangle)
    {
      GLint y_clip_pos = std::max(
        (GLint) (m_camera.viewport().y -
                 (u32) (cmd.clip_rectangle->pos.y + cmd.clip_rectangle->dimensions.y)),
        0
      );
      glScissor(
        (GLint) cmd.clip_rectangle->pos.x,
        y_clip_pos,
        (GLint) cmd.clip_rectangle->dimensions.x,
        (GLint) cmd.clip_rectangle->dimensions.y
      );
    }
    else
    {
      glScissor(0, 0, (GLint) m_camera.viewport().x, (GLint) m_camera.viewport().y);
    }

    glDrawElementsInstanced(
      gl_primitive(quad.primitive),
      (GLsizei) quad.submeshes[0].index_count,
      GL_UNSIGNED_INT,
      (void*) (quad.submeshes[0].index_offset * sizeof(u32)),
      (GLsizei) batch_size
    );

    cmd_idx += batch_size;
    shader.reset_texture_slot();
  }
}

void Pass::append(const Cmd2D& cmd)
{
  m_cmds_2d.push_back(cmd);
}

void Pass::append(const std::vector<Cmd2D>& cmd)
{
  m_cmds_2d.insert(m_cmds_2d.end(), cmd.begin(), cmd.end());
}

void Pass::append(const Cmd3D& cmd)
{

  m_cmds_3d.push_back(cmd);
}

void Pass::append(const std::vector<Cmd3D>& cmd)
{
  m_cmds_3d.insert(m_cmds_3d.end(), cmd.begin(), cmd.end());
}

void Pass::set_light(const vec3& pos, const vec3& color)
{
  m_light.pos = pos;
  m_light.color = color;
}

static mat4 get_transform(const vec3& pos, const vec3& size, f32 rotation)
{
  mat4 transform{1.0f};
  transform = rotate(transform, rotation, {0.0f, 1.0f, 0.0f});
  transform = translate(transform, pos);
  transform = scale(transform, size);
  return transform;
}

// NOTE: 2D

Cmd2D quad(const vec3& pos, const vec2& size, const Options2D& args)
{
  ASSERT(
    args.corner_radius >= 0.0f && args.corner_radius <= 1.0f,
    "Invalid corner_radius range provided"
  );
  return {
    .texture = render_data.blank_texture,
    .z_idx = pos.z,
    .instance_data =
      {
        .transform = get_transform(
          {pos.x + (size.x / 2.0f), pos.y + (size.y / 2.0f), pos.z},
          {size.x, size.y, 1.0f},
          0.0f
        ),
        .tint = args.tint,
        .corner_radius = args.corner_radius,
      },
    .clip_rectangle = args.clip_rectangle,
  };
}

Cmd2D texture(TextureHandle texture, const vec3& pos, const vec2& size, const Options2D& args)
{
  return {
    .texture = texture,
    .z_idx = pos.z,
    .instance_data =
      {
        .transform = get_transform(
          {pos.x + (size.x / 2.0f), pos.y + (size.y / 2.0f), pos.z},
          {size.x, size.y, 1.0f},
          0.0f
        ),
        .tint = args.tint,
        .corner_radius = args.corner_radius,
      },
    .clip_rectangle = args.clip_rectangle,
  };
}

Cmd2D texture_part(
  TextureHandle texture,
  const vec3& pos,
  const vec2& size,
  const vec2& in_texture_pos,
  const vec2& in_texture_size,
  const Options2D& args
)
{
  uvec2 dims = AssetManager::instance().get(texture).dimensions;
  vec2 dims_f32 = {(f32) dims.x, (f32) dims.y};
  return {
    .texture = texture,
    .z_idx = pos.z,
    .instance_data =
      {
        .transform = get_transform(
          {pos.x + (size.x / 2.0f), pos.y + (size.y / 2.0f), pos.z},
          {size.x, size.y, 1.0f},
          0.0f
        ),
        .tint = args.tint,
        .uv_scale = in_texture_size / dims_f32,
        .uv_offset = in_texture_pos / dims_f32,
        .corner_radius = args.corner_radius,
      },
    .clip_rectangle = args.clip_rectangle,
  };
}

// NOTE: 3D

std::vector<Cmd3D> mesh(MeshHandle handle, const vec3& pos, f32 rotation, const vec3& tint)
{
  std::vector<Cmd3D> out{};
  auto& mesh = AssetManager::instance().get(handle);
  for (usize i = 0; i < mesh.submeshes.size(); ++i)
  {
    out.push_back({
      .material = mesh.submeshes[i].material,
      .mesh = handle,
      .submesh_idx = i,
      .instance_data = {
        .transform = get_transform(pos, {1.0f, 1.0f, 1.0f}, rotation),
        .tint = {tint.x, tint.y, tint.z, 1.0f},
      },
    });
  }
  return out;
}

Cmd3D cube_wires(const vec3& pos, const vec3& size, const vec3& color)
{
  return {
    .material = AssetManager::instance().get(render_data.cube_wires).submeshes[0].material,
    .mesh = render_data.cube_wires,
    .submesh_idx = 0,
    .instance_data = {
      .transform = get_transform(pos, size, 0.0f),
      .tint = {color.x, color.y, color.z, 1.0f},
    },
  };
}

Cmd3D ring(const vec3& pos, f32 radius, const vec3& color)
{
  auto diameter = 2.0f * radius;
  return {
    .material = AssetManager::instance().get(render_data.ring).submeshes[0].material,
    .mesh = render_data.ring,
    .submesh_idx = 0,
    .instance_data = {
      .transform = get_transform(pos, {diameter, 1.0f, diameter}, 0.0f),
      .tint = {color.x, color.y, color.z, 1.0f},
    },
  };
}

Cmd3D line(const vec3& pos, f32 length, f32 rotation, const vec3& color)
{
  return {
    .material = AssetManager::instance().get(render_data.line).submeshes[0].material,
    .mesh = render_data.line,
    .submesh_idx = 0,
    .instance_data = {
      .transform = get_transform(pos, {length, 1.0f, length}, rotation),
      .tint = {color.x, color.y, color.z, 1.0f},
    },
  };
}

static constexpr Vertex cube_vertices[] = {
  {{-0.500000, 0.500000, 0.500000}, {-1.000000, -0.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-1.000000, -0.000000, -0.000000}, {1.000000, 0.000000}},
  {{-0.500000, -0.500000, 0.500000}, {-1.000000, -0.000000, -0.000000}, {0.000000, 0.000000}},
  {{-0.500000, 0.500000, -0.500000}, {-0.000000, -0.000000, -1.000000}, {0.000000, 1.000000}},
  {{0.500000, -0.500000, -0.500000}, {-0.000000, -0.000000, -1.000000}, {1.000000, 0.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-0.000000, -0.000000, -1.000000}, {0.000000, 0.000000}},
  {{0.500000, 0.500000, -0.500000}, {1.000000, -0.000000, -0.000000}, {1.000000, 1.000000}},
  {{0.500000, -0.500000, 0.500000}, {1.000000, -0.000000, -0.000000}, {0.000000, 0.000000}},
  {{0.500000, -0.500000, -0.500000}, {1.000000, -0.000000, -0.000000}, {1.000000, 0.000000}},
  {{0.500000, 0.500000, 0.500000}, {-0.000000, -0.000000, 1.000000}, {1.000000, 1.000000}},
  {{-0.500000, -0.500000, 0.500000}, {-0.000000, -0.000000, 1.000000}, {0.000000, 0.000000}},
  {{0.500000, -0.500000, 0.500000}, {-0.000000, -0.000000, 1.000000}, {1.000000, 0.000000}},
  {{0.500000, -0.500000, -0.500000}, {-0.000000, -1.000000, -0.000000}, {1.000000, 1.000000}},
  {{-0.500000, -0.500000, 0.500000}, {-0.000000, -1.000000, -0.000000}, {0.000000, 0.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-0.000000, -1.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, 0.500000, -0.500000}, {-0.000000, 1.000000, -0.000000}, {0.000000, 1.000000}},
  {{0.500000, 0.500000, 0.500000}, {-0.000000, 1.000000, -0.000000}, {1.000000, 0.000000}},
  {{0.500000, 0.500000, -0.500000}, {-0.000000, 1.000000, -0.000000}, {1.000000, 1.000000}},
  {{-0.500000, 0.500000, -0.500000}, {-1.000000, -0.000000, -0.000000}, {1.000000, 1.000000}},
  {{0.500000, 0.500000, -0.500000}, {-0.000000, -0.000000, -1.000000}, {1.000000, 1.000000}},
  {{0.500000, 0.500000, 0.500000}, {1.000000, -0.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, 0.500000, 0.500000}, {-0.000000, -0.000000, 1.000000}, {0.000000, 1.000000}},
  {{0.500000, -0.500000, 0.500000}, {-0.000000, -1.000000, -0.000000}, {1.000000, 0.000000}},
  {{-0.500000, 0.500000, 0.500000}, {-0.000000, 1.000000, -0.000000}, {0.000000, 0.000000}},
};
static constexpr u32 cube_wires_indices[] = {1, 2, 7, 4, 1, 3, 21, 2, 21, 9, 7, 9, 17, 4, 17, 3};

static constexpr Vertex ring_vertices[] = {
  {{0.000000f, 0.000000f, -0.500000f}, {}, {}},  {{-0.097545f, 0.000000f, -0.490393f}, {}, {}},
  {{-0.191342f, 0.000000f, -0.461940f}, {}, {}}, {{-0.277785f, 0.000000f, -0.415735f}, {}, {}},
  {{-0.353553f, 0.000000f, -0.353553f}, {}, {}}, {{-0.415735f, 0.000000f, -0.277785f}, {}, {}},
  {{-0.461940f, 0.000000f, -0.191342f}, {}, {}}, {{-0.490393f, 0.000000f, -0.097545f}, {}, {}},
  {{-0.500000f, 0.000000f, 0.000000f}, {}, {}},  {{-0.490393f, 0.000000f, 0.097545f}, {}, {}},
  {{-0.461940f, 0.000000f, 0.191342f}, {}, {}},  {{-0.415735f, 0.000000f, 0.277785f}, {}, {}},
  {{-0.353553f, 0.000000f, 0.353553f}, {}, {}},  {{-0.277785f, 0.000000f, 0.415735f}, {}, {}},
  {{-0.191342f, 0.000000f, 0.461940f}, {}, {}},  {{-0.097545f, 0.000000f, 0.490393f}, {}, {}},
  {{0.000000f, 0.000000f, 0.500000f}, {}, {}},   {{0.097545f, 0.000000f, 0.490393f}, {}, {}},
  {{0.191342f, 0.000000f, 0.461940f}, {}, {}},   {{0.277785f, 0.000000f, 0.415735f}, {}, {}},
  {{0.353553f, 0.000000f, 0.353553f}, {}, {}},   {{0.415735f, 0.000000f, 0.277785f}, {}, {}},
  {{0.461940f, 0.000000f, 0.191342f}, {}, {}},   {{0.490393f, 0.000000f, 0.097545f}, {}, {}},
  {{0.500000f, 0.000000f, 0.000000f}, {}, {}},   {{0.490393f, 0.000000f, -0.097545f}, {}, {}},
  {{0.461940f, 0.000000f, -0.191342f}, {}, {}},  {{0.415735f, 0.000000f, -0.277785f}, {}, {}},
  {{0.353553f, 0.000000f, -0.353553f}, {}, {}},  {{0.277785f, 0.000000f, -0.415735f}, {}, {}},
  {{0.191342f, 0.000000f, -0.461940f}, {}, {}},  {{0.097545f, 0.000000f, -0.490393f}, {}, {}},
};
static constexpr u32 ring_indices[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                       11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                       22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0};

static constexpr Vertex line_vertices[] = {
  {{0.0f, 0.0f, 0.0f}, {}, {}},
  {{0.0f, 0.0f, 1.0f}, {}, {}},
};
static constexpr u32 line_indices[] = {0, 1};

static constexpr Vertex quad_vertices[] = {
  {{0.5f, 0.5f, 0.0f}, {}, {1.0f, 1.0f}},
  {{0.5f, -0.5f, 0.0f}, {}, {1.0f, 0.0f}},
  {{-0.5f, -0.5f, 0.0f}, {}, {0.0f, 0.0f}},
  {{-0.5f, 0.5f, 0.0f}, {}, {0.0f, 1.0f}},
};
static constexpr u32 quad_indices[] = {0, 1, 3, 1, 2, 3};

static constexpr u8 blank_texture_data[] = {0xFF, 0xFF, 0xFF, 0xFF};
static constexpr uvec2 blank_texture_dimensions = {1, 1};

static MeshHandle static_model_init(
  std::vector<Vertex>&& vertices,
  std::vector<u32>&& indices,
  RenderPrimitive primitive
)
{
  Material material = {};
  material.diffuse_color = {1.0f, 1.0f, 1.0f};
  auto material_handle = AssetManager::instance().set(std::move(material));
  return AssetManager::instance().set(
    Mesh{
      std::move(vertices),
      std::move(indices),
      {{0, indices.size(), material_handle}},
      primitive,
      render_data
    }
  );
}

void init()
{
  auto& assets = AssetManager::instance();

  render_data.camera_2d = Camera{CameraDescription{
    .type = CameraType::ORTHOGRAPHIC,
    .pos = {0, 0, 1},
    .yaw = -90,
    .pitch = 0,
    .using_vertical_fov = true,
    .fov = 0.25f * std::numbers::pi_v<f32>,
    .near_plane = 0.0f,
    .far_plane = 10.0f
  }};

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);

  glGenBuffers(1, &render_data.camera_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, render_data.camera_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Camera), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, Data::UBO_INDEX_CAMERA, render_data.camera_ubo);

  glGenBuffers(1, &render_data.lights_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, render_data.lights_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Light), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, Data::UBO_INDEX_LIGHTS, render_data.lights_ubo);

  glGenBuffers(1, &render_data.instance_data_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, render_data.instance_data_buffer);
  glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(InstanceData), nullptr, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  render_data.default_shader = assets.set({"shaders/shader.vert", "shaders/default.frag"});
  render_data.quads_shader = assets.set({"shaders/quads.vert", "shaders/quads.frag"});
  render_data.lighting_shader = assets.set({"shaders/shader.vert", "shaders/lighting.frag"});

  render_data.cube_wires = static_model_init(
    {cube_vertices, cube_vertices + ARRAY_SIZE(cube_vertices)},
    {cube_wires_indices, cube_wires_indices + ARRAY_SIZE(cube_wires_indices)},
    RenderPrimitive::LINE_STRIP
  );
  render_data.ring = static_model_init(
    {ring_vertices, ring_vertices + ARRAY_SIZE(ring_vertices)},
    {ring_indices, ring_indices + ARRAY_SIZE(ring_indices)},
    RenderPrimitive::LINE_STRIP
  );
  render_data.line = static_model_init(
    {line_vertices, line_vertices + ARRAY_SIZE(line_vertices)},
    {line_indices, line_indices + ARRAY_SIZE(line_indices)},
    RenderPrimitive::LINE_STRIP
  );
  render_data.quad = static_model_init(
    {quad_vertices, quad_vertices + ARRAY_SIZE(quad_vertices)},
    {quad_indices, quad_indices + ARRAY_SIZE(quad_indices)},
    RenderPrimitive::TRIANGLES
  );

  render_data.blank_texture = assets.set(
    Texture{
      Image{blank_texture_data, blank_texture_dimensions},
      WrapOption::REPEAT,
      WrapOption::REPEAT,
      FilterOption::NEAREST,
      FilterOption::NEAREST
    }
  );

  assets.bind_render_data(render_data);
}

void create_framebuffer(TextureHandle texture)
{
  u32 id{};
  glGenFramebuffers(1, &id);
  glBindFramebuffer(GL_FRAMEBUFFER, id);
  glFramebufferTexture(
    GL_FRAMEBUFFER,
    GL_DEPTH_ATTACHMENT,
    AssetManager::instance().get(texture).handle(),
    0
  );
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  render_data.framebuffers.insert_or_assign(texture, id);
}

}
