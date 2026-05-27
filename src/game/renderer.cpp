#include "renderer.h"

#include <algorithm>

#include "base/math.h"
#include "base/errors.h"
#include "os/gl_functions.h"
#include "assets.h"
#include "camera.h"

static Render_Data g_render_data{};

struct STD140Camera
{
  mat4 proj_view{};
  vec3 view_pos{};
  float far_plane{};
};

struct STD140Light
{
  vec4 pos{};
  vec4 color{};
  f32 constant{};
  f32 linear{};
  f32 quadratic{};
  f32 _pad{};
};

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
static constexpr vec2 blank_texture_dimensions = {1, 1};

static MeshHandle static_model_init(
  const std::vector<Vertex>& vertices,
  const std::vector<u32>& indices,
  RenderPrimitive primitive,
  AssetStore& assets
)
{
  Material material = {};
  material.diffuse_color = {1.0f, 1.0f, 1.0f};
  auto material_handle = asset_set(assets, material);
  return asset_set(
    assets,
    mesh_init(vertices, indices, {{0, indices.size(), material_handle}}, primitive, g_render_data)
  );
}

void render_init(AssetStore& assets)
{
  g_render_data.camera_2d = {
    .type = CAMERA_TYPE_ORTHOGRAPHIC,
    .pos = {0, 0, 1},
    .prev_pos = {0, 0, 1},
    .yaw = -90,
    .pitch = 0,
    .fov_type = FOV_TYPE_VERTICAL,
    .fov = 0.25f * std::numbers::pi_v<f32>,
    .near_plane = 0.0f,
    .far_plane = 10.0f,
  };
  camera_update_vectors(g_render_data.camera_2d);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);

  glGenBuffers(1, &g_render_data.camera_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, g_render_data.camera_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Camera), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, RENDER_UBO_INDEX_CAMERA, g_render_data.camera_ubo);

  glGenBuffers(1, &g_render_data.lights_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, g_render_data.lights_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Light), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, RENDER_UBO_INDEX_LIGHTS, g_render_data.lights_ubo);

  glGenBuffers(1, &g_render_data.instance_data_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, g_render_data.instance_data_buffer);
  glBufferData(
    GL_ARRAY_BUFFER,
    RENDER_MAX_INSTANCES * sizeof(Render_InstanceData),
    nullptr,
    GL_STATIC_DRAW
  );
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  {
    auto shader = shader_from_file("shaders/shader.vert", "shaders/default.frag");
    if (shader)
    {
      g_render_data.default_shader = asset_set(assets, *shader);
    }
    else
    {
      REPORT_ERROR(shader.error());
    }
  }
  {
    auto shader = shader_from_file("shaders/quads.vert", "shaders/quads.frag");
    if (shader)
    {
      g_render_data.quads_shader = asset_set(assets, *shader);
    }
    else
    {
      REPORT_ERROR(shader.error());
    }
  }
  {
    auto shader = shader_from_file("shaders/shader.vert", "shaders/lighting.frag");
    if (shader)
    {
      g_render_data.lighting_shader = asset_set(assets, *shader);
    }
    else
    {
      REPORT_ERROR(shader.error());
    }
  }

  g_render_data.cube_wires = static_model_init(
    {cube_vertices, cube_vertices + ARRAY_SIZE(cube_vertices)},
    {cube_wires_indices, cube_wires_indices + ARRAY_SIZE(cube_wires_indices)},
    RENDER_PRIMITIVE_LINE_STRIP,
    assets
  );
  g_render_data.ring = static_model_init(
    {ring_vertices, ring_vertices + ARRAY_SIZE(ring_vertices)},
    {ring_indices, ring_indices + ARRAY_SIZE(ring_indices)},
    RENDER_PRIMITIVE_LINE_STRIP,
    assets
  );
  g_render_data.line = static_model_init(
    {line_vertices, line_vertices + ARRAY_SIZE(line_vertices)},
    {line_indices, line_indices + ARRAY_SIZE(line_indices)},
    RENDER_PRIMITIVE_LINE_STRIP,
    assets
  );
  g_render_data.quad = static_model_init(
    {quad_vertices, quad_vertices + ARRAY_SIZE(quad_vertices)},
    {quad_indices, quad_indices + ARRAY_SIZE(quad_indices)},
    RENDER_PRIMITIVE_TRIANGLES,
    assets
  );

  g_render_data.blank_texture = asset_set(
    assets,
    texture_from_image(
      image_init(blank_texture_data, blank_texture_dimensions),
      WRAP_OPTION_REPEAT,
      WRAP_OPTION_REPEAT,
      FILTER_OPTION_NEAREST,
      FILTER_OPTION_NEAREST
    )
  );

  asset_store_bind_render_data(assets, g_render_data);
}

void render_create_framebuffer(TextureHandle texture, AssetStore& assets)
{
  u32 id{};
  glGenFramebuffers(1, &id);
  glBindFramebuffer(GL_FRAMEBUFFER, id);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, asset_get(assets, texture).id, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  g_render_data.framebuffers.insert_or_assign(texture, id);
}

static GLenum render_primitive_to_gl_primitive(RenderPrimitive primitive)
{
  switch (primitive)
  {
    case RENDER_PRIMITIVE_TRIANGLES:
    {
      return GL_TRIANGLES;
    }
    break;
    case RENDER_PRIMITIVE_LINE_STRIP:
    {
      return GL_LINE_STRIP;
    }
    break;
  }
}

void render_pass_finish(Render_Pass& pass, AssetStore& assets)
{
  // NOTE: 3D
  std::ranges::sort(
    pass.cmds_3d,
    [](const Render_Cmd3D& a, const Render_Cmd3D& b) -> bool
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

  if (pass.flags & PASS_OVERRIDDEN_FRAMEBUFFER)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, pass.framebuffer);
  }
  else
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  glDisable(GL_SCISSOR_TEST);
  glViewport(0, 0, (GLsizei) pass.camera->viewport.x, (GLsizei) pass.camera->viewport.y);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  STD140Camera camera_std140 = {};
  camera_std140.view_pos = pass.camera->pos;
  camera_std140.proj_view = camera_projection(*pass.camera) * camera_view(*pass.camera, pass.t);
  camera_std140.far_plane = pass.camera->far_plane;
  glBindBuffer(GL_UNIFORM_BUFFER, g_render_data.camera_ubo);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(STD140Camera), &camera_std140);

  STD140Light light_std140 = {};
  light_std140.pos = {pass.light.pos.x, pass.light.pos.y, pass.light.pos.z, 1.0f};
  light_std140.color = {pass.light.color.x, pass.light.color.y, pass.light.color.z, 1.0f};
  light_std140.constant = RENDER_LIGHT_ATTENUATION_CONSTANT;
  light_std140.linear = RENDER_LIGHT_ATTENUATION_LINEAR;
  light_std140.quadratic = RENDER_LIGHT_ATTENUATION_QUADRATIC;
  glBindBuffer(GL_UNIFORM_BUFFER, g_render_data.lights_ubo);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(STD140Light), &light_std140);

  for (usize cmd_idx = 0; cmd_idx < pass.cmds_3d.size();)
  {
    const auto& cmd = pass.cmds_3d[cmd_idx];
    const auto& mesh = asset_get(assets, cmd.mesh);
    const auto& submesh = mesh.submeshes[cmd.submesh_idx];
    const auto& material = asset_get(assets, cmd.material);

    usize batch_idx = cmd_idx + 1;
    while ((batch_idx < pass.cmds_3d.size() && batch_idx - cmd_idx < RENDER_MAX_INSTANCES) &&
           cmd.mesh.id == pass.cmds_3d[batch_idx].mesh.id &&
           cmd.submesh_idx == pass.cmds_3d[batch_idx].submesh_idx &&
           cmd.material.id == pass.cmds_3d[batch_idx].material.id)
    {
      ++batch_idx;
    }
    const auto batch_size = batch_idx - cmd_idx;

    std::vector<Render_InstanceData> instance_data{};
    instance_data.reserve(batch_size);
    for (usize i = cmd_idx; i < batch_idx; ++i)
    {
      instance_data.push_back(pass.cmds_3d[i].instance_data);
    }

    ShaderHandle handle = g_render_data.default_shader;
    if (pass.flags & PASS_OVERRIDDEN_SHADER)
    {
      handle = pass.overridden_shader;
    }
    else if (material.specular_exponent != 0.0f)
    {
      handle = g_render_data.lighting_shader;
    }
    auto& shader = asset_get(assets, handle);

    shader_use(shader);
    if (pass.on_shader_bind)
    {
      pass.on_shader_bind(shader);
    }

    shader_set(shader, "material.ambient", pass.ambient_color);
    shader_set(shader, "material.diffuse", material.diffuse_color);
    shader_set(shader, "material.specular", material.specular_color);
    shader_set(shader, "material.specular_exponent", material.specular_exponent);
    if (material.diffuse_map.id)
    {
      shader_set(shader, "material.diffuse_map", material.diffuse_map, assets);
      shader_set(shader, "material.use_diffuse_map", true);
    }
    else
    {
      shader_set(shader, "material.diffuse_map", g_render_data.blank_texture, assets);
      shader_set(shader, "material.use_diffuse_map", false);
    }

    glBindBuffer(GL_ARRAY_BUFFER, g_render_data.instance_data_buffer);
    glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      (GLsizeiptr) (instance_data.size() * sizeof(Render_InstanceData)),
      instance_data.data()
    );

    mesh_use(mesh);

    glDrawElementsInstanced(
      render_primitive_to_gl_primitive(mesh.primitive),
      (GLsizei) submesh.index_count,
      GL_UNSIGNED_INT,
      (void*) (submesh.index_offset * sizeof(u32)),
      (GLsizei) batch_size
    );

    cmd_idx += batch_size;
    shader_reset_texture_slot(shader);
  }

  // NOTE: 2D
  std::ranges::sort(
    pass.cmds_2d,
    [](const Render_Cmd2D& a, const Render_Cmd2D& b)
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
  g_render_data.camera_2d.viewport = pass.camera->viewport;
  camera_std140.view_pos = g_render_data.camera_2d.pos;
  camera_std140.proj_view =
    camera_projection(g_render_data.camera_2d) * camera_view(g_render_data.camera_2d, pass.t);
  camera_std140.far_plane = g_render_data.camera_2d.far_plane;
  glBindBuffer(GL_UNIFORM_BUFFER, g_render_data.camera_ubo);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(STD140Camera), &camera_std140);

  const auto& quad = asset_get(assets, g_render_data.quad);
  for (usize cmd_idx = 0; cmd_idx < pass.cmds_2d.size();)
  {
    const auto& cmd = pass.cmds_2d[cmd_idx];

    usize batch_idx = cmd_idx + 1;
    while ((batch_idx < pass.cmds_2d.size() && batch_idx - cmd_idx < RENDER_MAX_INSTANCES) &&
           cmd.texture == pass.cmds_2d[batch_idx].texture &&
           cmd.clip_rectangle == pass.cmds_2d[batch_idx].clip_rectangle)
    {
      ++batch_idx;
    }
    const auto batch_size = batch_idx - cmd_idx;

    std::vector<Render_InstanceData> instance_data{};
    instance_data.reserve(batch_size);
    for (usize i = cmd_idx; i < batch_idx; ++i)
    {
      instance_data.push_back(pass.cmds_2d[i].instance_data);
    }

    auto& shader = asset_get(assets, g_render_data.quads_shader);
    shader_use(shader);
    shader_set(shader, "diffuse_map", cmd.texture, assets);

    glBindBuffer(GL_ARRAY_BUFFER, g_render_data.instance_data_buffer);
    glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      (GLsizeiptr) (instance_data.size() * sizeof(Render_InstanceData)),
      instance_data.data()
    );

    mesh_use(quad);

    if (cmd.clip_rectangle)
    {
      GLint y_clip_pos = std::max(
        (GLint) (g_render_data.camera_2d.viewport.y -
                 (f32) (cmd.clip_rectangle->pos.y + cmd.clip_rectangle->dimensions.y)),
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
      glScissor(
        0,
        0,
        (GLint) g_render_data.camera_2d.viewport.x,
        (GLint) g_render_data.camera_2d.viewport.y
      );
    }

    glDrawElementsInstanced(
      render_primitive_to_gl_primitive(quad.primitive),
      (GLsizei) quad.submeshes[0].index_count,
      GL_UNSIGNED_INT,
      (void*) (quad.submeshes[0].index_offset * sizeof(u32)),
      (GLsizei) batch_size
    );

    cmd_idx += batch_size;
    shader_reset_texture_slot(shader);
  }
}

void render_pass_render_to(Render_Pass& pass, TextureHandle texture)
{
  ASSERT(
    g_render_data.framebuffers.contains(texture),
    "No framebuffer associated with this texture"
  );
  pass.flags |= PASS_OVERRIDDEN_FRAMEBUFFER;
  pass.framebuffer = g_render_data.framebuffers[texture];
}

void render_pass_override_shader(Render_Pass& pass, ShaderHandle shader)
{
  pass.flags |= PASS_OVERRIDDEN_SHADER;
  pass.overridden_shader = shader;
}

void render_pass_on_shader_bind(Render_Pass& pass, Render_PassOnShaderBindCallback&& on_bind)
{
  pass.on_shader_bind = std::move(on_bind);
}

void render_pass_set_light(Render_Pass& pass, const vec3& pos, const vec3& color)
{
  pass.light.pos = pos;
  pass.light.color = color;
}

static mat4 get_transform(const vec3& pos, const vec3& size, f32 rotation)
{
  auto transform = unit_matrix();
  transform = rotate(transform, rotation, {0, 1, 0});
  transform = translate(transform, pos);
  transform = scale(transform, size);
  return transform;
}

// NOTE: 2D

Render_Cmd2D render_quad(const vec3& pos, const vec2& size, const Render_Options2D& args)
{
  ASSERT(
    args.corner_radius >= 0.0f && args.corner_radius <= 1.0f,
    "Invalid corner_radius range provided"
  );
  return {
    .texture = g_render_data.blank_texture,
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

Render_Cmd2D render_texture(
  TextureHandle texture,
  const vec3& pos,
  const vec2& size,
  const Render_Options2D& args
)
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

Render_Cmd2D render_texture_part(
  TextureHandle texture,
  const vec3& pos,
  const vec2& size,
  const vec2& in_texture_pos,
  const vec2& in_texture_size,
  const Render_Options2D& args,
  AssetStore& assets
)
{
  vec2 dims = asset_get(assets, texture).dimensions;
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
        .uv_scale = in_texture_size / dims,
        .uv_offset = in_texture_pos / dims,
        .corner_radius = args.corner_radius,
      },
    .clip_rectangle = args.clip_rectangle,
  };
}

// NOTE: 3D

void render_mesh(
  std::vector<Render_Cmd3D>& cmds,
  MeshHandle handle,
  const vec3& pos,
  f32 rotation,
  const vec3& tint,
  AssetStore& assets
)
{
  auto& mesh = asset_get(assets, handle);
  for (usize i = 0; i < mesh.submeshes.size(); ++i)
  {
    cmds.push_back({
      .material = mesh.submeshes[i].material,
      .mesh = handle,
      .submesh_idx = i,
      .instance_data =
        {
          .transform = get_transform(pos, {1.0f, 1.0f, 1.0f}, rotation),
          .tint = {tint.x, tint.y, tint.z, 1.0f},
        },
    });
  }
}

Render_Cmd3D
render_cube_wires(const vec3& pos, const vec3& size, const vec3& color, AssetStore& assets)
{
  return {
    .material = asset_get(assets, g_render_data.cube_wires).submeshes[0].material,
    .mesh = g_render_data.cube_wires,
    .submesh_idx = 0,
    .instance_data =
      {
        .transform = get_transform(pos, size, 0.0f),
        .tint = {color.x, color.y, color.z, 1.0f},
      },
  };
}

Render_Cmd3D render_ring(const vec3& pos, f32 radius, const vec3& color, AssetStore& assets)
{
  auto diameter = 2.0f * radius;
  return {
    .material = asset_get(assets, g_render_data.ring).submeshes[0].material,
    .mesh = g_render_data.ring,
    .submesh_idx = 0,
    .instance_data =
      {
        .transform = get_transform(pos, {diameter, 1.0f, diameter}, 0.0f),
        .tint = {color.x, color.y, color.z, 1.0f},
      },
  };
}

Render_Cmd3D
render_line(const vec3& pos, f32 length, f32 rotation, const vec3& color, AssetStore& assets)
{
  return {
    .material = asset_get(assets, g_render_data.line).submeshes[0].material,
    .mesh = g_render_data.line,
    .submesh_idx = 0,
    .instance_data =
      {
        .transform = get_transform(pos, {length, 1.0f, length}, rotation),
        .tint = {color.x, color.y, color.z, 1.0f},
      },
  };
}
