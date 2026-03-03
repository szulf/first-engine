#include "renderer.h"

#include <algorithm>

#include "game/assets.h"
#include "os/gl_functions.h"

mat4 get_transform_(const vec3& pos, const vec3& size, f32 rotation)
{
  mat4 transform{1.0f};
  transform = rotate(transform, rotation, {0.0f, 1.0f, 0.0f});
  transform = translate(transform, pos);
  transform = scale(transform, size);
  return transform;
}

void RenderPass::draw_mesh(MeshHandle handle, const vec3& pos, f32 rotation, const vec3& tint)
{
  auto transform = get_transform_(pos, {1.0f, 1.0f, 1.0f}, rotation);
  auto& mesh = AssetManager::instance().get(handle);
  for (usize i = 0; i < mesh.submeshes.size(); ++i)
  {
    RenderItem item = {};
    item.mesh = handle;
    item.submesh_idx = i;
    item.material = mesh.submeshes[i].material;
    item.instance_data.transform = transform;
    item.instance_data.tint = tint;
    m_items.push_back(item);
  }
}

void RenderPass::draw_cube_wires(const vec3& pos, const vec3& size, const vec3& color)
{
  auto transform = get_transform_(pos, size, 0.0f);
  RenderItem item = {};
  item.mesh = m_render_data.cube_wires;
  item.submesh_idx = 0;
  item.material = AssetManager::instance().get(m_render_data.cube_wires).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  m_items.push_back(item);
}

void RenderPass::draw_ring(const vec3& pos, f32 radius, const vec3& color)
{
  auto diameter = 2.0f * radius;
  auto transform = get_transform_(pos, {diameter, 1.0f, diameter}, 0.0f);
  RenderItem item = {};
  item.mesh = m_render_data.ring;
  item.submesh_idx = 0;
  item.material = AssetManager::instance().get(m_render_data.ring).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  m_items.push_back(item);
}

void RenderPass::draw_line(const vec3& pos, f32 length, f32 rotation, const vec3& color)
{
  auto transform = get_transform_(pos, {length, 1.0f, length}, rotation);
  RenderItem item = {};
  item.mesh = m_render_data.line;
  item.submesh_idx = 0;
  item.material = AssetManager::instance().get(m_render_data.line).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  m_items.push_back(item);
}

void RenderPass::set_light(const vec3& pos, const vec3& color)
{
  m_light.pos = pos;
  m_light.color = color;
}

void RenderPass::use_shadow_map(const Camera& camera_)
{
  m_shadow_map_camera = &camera_;
}

static GLenum get_primitive_(RenderPrimitive primitive)
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

void RenderPass::finish()
{
  auto& assets = AssetManager::instance();
  std::ranges::sort(
    m_items,
    [](const RenderItem& a, const RenderItem& b) -> bool
    {
      if (a.material.id != b.material.id)
      {
        return a.material.id > b.material.id;
      }
      if (a.mesh.id != b.mesh.id)
      {
        return a.mesh.id > b.mesh.id;
      }
      return a.submesh_idx > b.submesh_idx;
    }
  );

  switch (m_type)
  {
    case RenderPassType::FORWARD:
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    break;
    case RenderPassType::POINT_SHADOW_MAP:
    {
      glBindFramebuffer(GL_FRAMEBUFFER, m_render_data.shadow_framebuffer_id);
    }
    break;
  }

  glViewport(0, 0, (GLsizei) m_camera.viewport().x, (GLsizei) m_camera.viewport().y);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    STD140Camera camera_std140 = {};
    camera_std140.view_pos = m_camera.pos();
    camera_std140.proj_view = m_camera.projection() * m_camera.look_at();
    camera_std140.far_plane = m_camera.far_plane();
    glBindBuffer(GL_UNIFORM_BUFFER, m_render_data.camera_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera_std140), &camera_std140);
  }

  {
    STD140Light light_std140 = {};
    light_std140.pos = {m_light.pos.x, m_light.pos.y, m_light.pos.z, 1.0f};
    light_std140.color = {m_light.color.x, m_light.color.y, m_light.color.z, 1.0f};
    light_std140.constant = Light::CONSTANT;
    light_std140.linear = Light::LINEAR;
    light_std140.quadratic = Light::QUADRATIC;
    glBindBuffer(GL_UNIFORM_BUFFER, m_render_data.lights_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(STD140Light), &light_std140);
  }

  for (usize item_idx = 0; item_idx < m_items.size();)
  {
    const auto& item = m_items[item_idx];
    const auto& mesh = assets.get(item.mesh);
    const auto& submesh = mesh.submeshes[item.submesh_idx];
    const auto& material = assets.get(item.material);

    usize batch_idx = item_idx + 1;
    while ((batch_idx < m_items.size() && batch_idx - item_idx < InstanceData::MAX) &&
           item.mesh.id == m_items[batch_idx].mesh.id &&
           item.submesh_idx == m_items[batch_idx].submesh_idx &&
           item.material.id == m_items[batch_idx].material.id)
    {
      ++batch_idx;
    }
    const auto batch_size = batch_idx - item_idx;

    std::vector<InstanceData> instance_data{};
    instance_data.reserve(batch_size);
    for (usize i = item_idx; i < batch_idx; ++i)
    {
      instance_data.push_back(m_items[i].instance_data);
    }

    ShaderHandle handle = m_render_data.default_shader;
    if (m_type == RenderPassType::POINT_SHADOW_MAP)
    {
      handle = m_render_data.shadow_depth_shader;
    }
    else if (material.specular_exponent != 0.0f)
    {
      handle = m_render_data.lighting_shader;
    }
    auto& shader = assets.get(handle);

    shader.use();

    {
      shader.set("material.ambient", m_ambient_color);
      shader.set("material.diffuse", material.diffuse_color);
      shader.set("material.specular", material.specular_color);
      shader.set("material.specular_exponent", material.specular_exponent);
      const auto& diffuse_map = assets.get(material.diffuse_map);
      diffuse_map.activate(0);
      shader.set("material.diffuse_map", 0);
    }

    if (m_type == RenderPassType::POINT_SHADOW_MAP)
    {
      mat4 light_proj_mat = m_camera.projection();
      std::array<mat4, 6> transforms = {
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{1.0f, 0.0f, 0.0f},
                           {0.0f, -1.0f, 0.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{-1.0f, 0.0f, 0.0f},
                           {0.0f, -1.0f, 0.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{0.0f, 1.0f, 0.0f},
                           {0.0f, 0.0f, 1.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{0.0f, -1.0f, 0.0f},
                           {0.0f, 0.0f, -1.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{0.0f, 0.0f, 1.0f},
                           {0.0f, -1.0f, 0.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{0.0f, 0.0f, -1.0f},
                           {0.0f, -1.0f, 0.0f}
                         ),
      };

      shader.set("shadow_matrices[0]", transforms[0]);
      shader.set("shadow_matrices[1]", transforms[1]);
      shader.set("shadow_matrices[2]", transforms[2]);
      shader.set("shadow_matrices[3]", transforms[3]);
      shader.set("shadow_matrices[4]", transforms[4]);
      shader.set("shadow_matrices[5]", transforms[5]);
    }

    if (m_shadow_map_camera)
    {
      glActiveTexture(GL_TEXTURE1);

      glBindTexture(GL_TEXTURE_CUBE_MAP, m_render_data.shadow_cubemap);
      shader.set("shadow_map", 1);

      shader.set("shadow_map_camera_far_plane", m_shadow_map_camera->far_plane());
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_render_data.instance_data_buffer);
    glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      (GLsizeiptr) (instance_data.size() * sizeof(InstanceData)),
      instance_data.data()
    );

    mesh.use();

    glDrawElementsInstanced(
      get_primitive_(mesh.primitive),
      (GLsizei) submesh.index_count,
      GL_UNSIGNED_INT,
      (void*) (submesh.index_offset * sizeof(u32)),
      (GLsizei) batch_size
    );

    item_idx += batch_size;
  }
}

static constexpr Vertex cube_vertices[] = {
  {{-0.500000, 0.500000, 0.500000},   {-1.000000, -0.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-1.000000, -0.000000, -0.000000}, {1.000000, 0.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-1.000000, -0.000000, -0.000000}, {0.000000, 0.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-0.000000, -0.000000, -1.000000}, {0.000000, 1.000000}},
  {{0.500000, -0.500000, -0.500000},  {-0.000000, -0.000000, -1.000000}, {1.000000, 0.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-0.000000, -0.000000, -1.000000}, {0.000000, 0.000000}},
  {{0.500000, 0.500000, -0.500000},   {1.000000, -0.000000, -0.000000},  {1.000000, 1.000000}},
  {{0.500000, -0.500000, 0.500000},   {1.000000, -0.000000, -0.000000},  {0.000000, 0.000000}},
  {{0.500000, -0.500000, -0.500000},  {1.000000, -0.000000, -0.000000},  {1.000000, 0.000000}},
  {{0.500000, 0.500000, 0.500000},    {-0.000000, -0.000000, 1.000000},  {1.000000, 1.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-0.000000, -0.000000, 1.000000},  {0.000000, 0.000000}},
  {{0.500000, -0.500000, 0.500000},   {-0.000000, -0.000000, 1.000000},  {1.000000, 0.000000}},
  {{0.500000, -0.500000, -0.500000},  {-0.000000, -1.000000, -0.000000}, {1.000000, 1.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-0.000000, -1.000000, -0.000000}, {0.000000, 0.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-0.000000, -1.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-0.000000, 1.000000, -0.000000},  {0.000000, 1.000000}},
  {{0.500000, 0.500000, 0.500000},    {-0.000000, 1.000000, -0.000000},  {1.000000, 0.000000}},
  {{0.500000, 0.500000, -0.500000},   {-0.000000, 1.000000, -0.000000},  {1.000000, 1.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-1.000000, -0.000000, -0.000000}, {1.000000, 1.000000}},
  {{0.500000, 0.500000, -0.500000},   {-0.000000, -0.000000, -1.000000}, {1.000000, 1.000000}},
  {{0.500000, 0.500000, 0.500000},    {1.000000, -0.000000, -0.000000},  {0.000000, 1.000000}},
  {{-0.500000, 0.500000, 0.500000},   {-0.000000, -0.000000, 1.000000},  {0.000000, 1.000000}},
  {{0.500000, -0.500000, 0.500000},   {-0.000000, -1.000000, -0.000000}, {1.000000, 0.000000}},
  {{-0.500000, 0.500000, 0.500000},   {-0.000000, 1.000000, -0.000000},  {0.000000, 0.000000}},
};

static constexpr u32 cube_wires_indices[] = {1, 2, 7, 4, 1, 3, 21, 2, 21, 9, 7, 9, 17, 4, 17, 3};

static constexpr Vertex ring_vertices[] = {
  {{0.000000f, 0.000000f, -0.500000f},  {}, {}},
  {{-0.097545f, 0.000000f, -0.490393f}, {}, {}},
  {{-0.191342f, 0.000000f, -0.461940f}, {}, {}},
  {{-0.277785f, 0.000000f, -0.415735f}, {}, {}},
  {{-0.353553f, 0.000000f, -0.353553f}, {}, {}},
  {{-0.415735f, 0.000000f, -0.277785f}, {}, {}},
  {{-0.461940f, 0.000000f, -0.191342f}, {}, {}},
  {{-0.490393f, 0.000000f, -0.097545f}, {}, {}},
  {{-0.500000f, 0.000000f, 0.000000f},  {}, {}},
  {{-0.490393f, 0.000000f, 0.097545f},  {}, {}},
  {{-0.461940f, 0.000000f, 0.191342f},  {}, {}},
  {{-0.415735f, 0.000000f, 0.277785f},  {}, {}},
  {{-0.353553f, 0.000000f, 0.353553f},  {}, {}},
  {{-0.277785f, 0.000000f, 0.415735f},  {}, {}},
  {{-0.191342f, 0.000000f, 0.461940f},  {}, {}},
  {{-0.097545f, 0.000000f, 0.490393f},  {}, {}},
  {{0.000000f, 0.000000f, 0.500000f},   {}, {}},
  {{0.097545f, 0.000000f, 0.490393f},   {}, {}},
  {{0.191342f, 0.000000f, 0.461940f},   {}, {}},
  {{0.277785f, 0.000000f, 0.415735f},   {}, {}},
  {{0.353553f, 0.000000f, 0.353553f},   {}, {}},
  {{0.415735f, 0.000000f, 0.277785f},   {}, {}},
  {{0.461940f, 0.000000f, 0.191342f},   {}, {}},
  {{0.490393f, 0.000000f, 0.097545f},   {}, {}},
  {{0.500000f, 0.000000f, 0.000000f},   {}, {}},
  {{0.490393f, 0.000000f, -0.097545f},  {}, {}},
  {{0.461940f, 0.000000f, -0.191342f},  {}, {}},
  {{0.415735f, 0.000000f, -0.277785f},  {}, {}},
  {{0.353553f, 0.000000f, -0.353553f},  {}, {}},
  {{0.277785f, 0.000000f, -0.415735f},  {}, {}},
  {{0.191342f, 0.000000f, -0.461940f},  {}, {}},
  {{0.097545f, 0.000000f, -0.490393f},  {}, {}},
};

static constexpr u32 ring_indices[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                       11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                       22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0};

static constexpr Vertex line_vertices[] = {
  {{0.0f, 0.0f, 0.0f}, {}, {}},
  {{0.0f, 0.0f, 1.0f}, {}, {}},
};

static constexpr u32 line_indices[] = {0, 1};

static MeshHandle static_model_init(
  RenderData& render_data,
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

Renderer::Renderer()
{
  glEnable(GL_DEPTH_TEST);

  glGenBuffers(1, &m_render_data.camera_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, m_render_data.camera_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Camera), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_CAMERA, m_render_data.camera_ubo);

  glGenBuffers(1, &m_render_data.lights_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, m_render_data.lights_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Light), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_LIGHTS, m_render_data.lights_ubo);

  glGenTextures(1, &m_render_data.shadow_cubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_render_data.shadow_cubemap);
  for (i32 i = 0; i < 6; ++i)
  {
    glTexImage2D(
      (GLenum) (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
      0,
      GL_DEPTH_COMPONENT,
      RenderData::SHADOW_CUBEMAP_DIMENSIONS.x,
      RenderData::SHADOW_CUBEMAP_DIMENSIONS.y,
      0,
      GL_DEPTH_COMPONENT,
      GL_FLOAT,
      nullptr
    );
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &m_render_data.shadow_framebuffer_id);
  glBindFramebuffer(GL_FRAMEBUFFER, m_render_data.shadow_framebuffer_id);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_render_data.shadow_cubemap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glGenBuffers(1, &m_render_data.instance_data_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_render_data.instance_data_buffer);
  glBufferData(GL_ARRAY_BUFFER, InstanceData::MAX * sizeof(InstanceData), nullptr, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_render_data.default_shader =
    AssetManager::instance().set({"shaders/shader.vert", "shaders/default.frag"});
  m_render_data.lighting_shader =
    AssetManager::instance().set({"shaders/shader.vert", "shaders/lighting.frag"});
  m_render_data.shadow_depth_shader = AssetManager::instance().set(
    {"shaders/shadow_depth.vert", "shaders/shadow_depth.frag", "shaders/shadow_depth.geom"}
  );

  m_render_data.cube_wires = static_model_init(
    m_render_data,
    std::vector<Vertex>{cube_vertices, cube_vertices + ARRAY_SIZE(cube_vertices)},
    std::vector<u32>{cube_wires_indices, cube_wires_indices + ARRAY_SIZE(cube_wires_indices)},
    RenderPrimitive::LINE_STRIP
  );
  m_render_data.ring = static_model_init(
    m_render_data,
    std::vector<Vertex>{ring_vertices, ring_vertices + ARRAY_SIZE(ring_vertices)},
    std::vector<u32>{ring_indices, ring_indices + ARRAY_SIZE(ring_indices)},
    RenderPrimitive::LINE_STRIP
  );
  m_render_data.line = static_model_init(
    m_render_data,
    std::vector<Vertex>{line_vertices, line_vertices + ARRAY_SIZE(line_vertices)},
    std::vector<u32>{line_indices, line_indices + ARRAY_SIZE(line_indices)},
    RenderPrimitive::LINE_STRIP
  );

  AssetManager::instance().bind_render_data(m_render_data);
}

RenderPass
Renderer::begin_pass(RenderPassType type, const Camera& camera, const vec3& ambient_color)
{
  return {m_render_data, type, camera, ambient_color};
}
