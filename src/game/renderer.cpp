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

void RenderPass::render_to(TextureHandle texture)
{
  ASSERT(
    m_render_data.framebuffers.contains(texture),
    "No framebuffer associated with this texture."
  );
  m_flags |= OVERRIDED_FRAMEBUFFER;
  m_framebuffer = m_render_data.framebuffers[texture];
}

void RenderPass::override_shader(ShaderHandle shader)
{
  m_flags |= OVERRIDED_SHADER;
  m_overidded_shader = shader;
}

void RenderPass::on_shader_bind(OnShaderBindCallback&& on_bind)
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

void RenderPass::finish()
{
  auto& assets = AssetManager::instance();
  std::ranges::sort(
    m_cmds,
    [](const RenderCmd& a, const RenderCmd& b) -> bool
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

  if (m_flags & OVERRIDED_FRAMEBUFFER)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  }
  else
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  glViewport(0, 0, (GLsizei) m_camera.viewport().x, (GLsizei) m_camera.viewport().y);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  STD140Camera camera_std140 = {};
  camera_std140.view_pos = m_camera.pos();
  camera_std140.proj_view = m_camera.projection() * m_camera.look_at();
  camera_std140.far_plane = m_camera.far_plane();
  glBindBuffer(GL_UNIFORM_BUFFER, m_render_data.camera_ubo);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera_std140), &camera_std140);

  STD140Light light_std140 = {};
  light_std140.pos = {m_light.pos.x, m_light.pos.y, m_light.pos.z, 1.0f};
  light_std140.color = {m_light.color.x, m_light.color.y, m_light.color.z, 1.0f};
  light_std140.constant = Light::CONSTANT;
  light_std140.linear = Light::LINEAR;
  light_std140.quadratic = Light::QUADRATIC;
  glBindBuffer(GL_UNIFORM_BUFFER, m_render_data.lights_ubo);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(STD140Light), &light_std140);

  for (usize item_idx = 0; item_idx < m_cmds.size();)
  {
    const auto& cmd = m_cmds[item_idx];
    const auto& mesh = assets.get(cmd.mesh);
    const auto& submesh = mesh.submeshes[cmd.submesh_idx];
    const auto& material = assets.get(cmd.material);

    usize batch_idx = item_idx + 1;
    while ((batch_idx < m_cmds.size() && batch_idx - item_idx < MAX_INSTANCES) &&
           cmd.mesh.id == m_cmds[batch_idx].mesh.id &&
           cmd.submesh_idx == m_cmds[batch_idx].submesh_idx &&
           cmd.material.id == m_cmds[batch_idx].material.id)
    {
      ++batch_idx;
    }
    const auto batch_size = batch_idx - item_idx;

    std::vector<InstanceData> instance_data{};
    instance_data.reserve(batch_size);
    for (usize i = item_idx; i < batch_idx; ++i)
    {
      instance_data.push_back(m_cmds[i].instance_data);
    }

    ShaderHandle handle = m_render_data.default_shader;
    if (m_flags & OVERRIDED_SHADER)
    {
      handle = m_overidded_shader;
    }
    else if (material.specular_exponent != 0.0f)
    {
      handle = m_render_data.lighting_shader;
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
    const auto& diffuse_map = assets.get(material.diffuse_map);
    diffuse_map.activate(0);
    shader.set("material.diffuse_map", 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_render_data.instance_data_buffer);
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

    item_idx += batch_size;
  }
}

void RenderPass::draw_mesh(MeshHandle handle, const vec3& pos, f32 rotation, const vec3& tint)
{
  auto transform = get_transform_(pos, {1.0f, 1.0f, 1.0f}, rotation);
  auto& mesh = AssetManager::instance().get(handle);
  for (usize i = 0; i < mesh.submeshes.size(); ++i)
  {
    RenderCmd cmd = {};
    cmd.mesh = handle;
    cmd.submesh_idx = i;
    cmd.material = mesh.submeshes[i].material;
    cmd.instance_data.transform = transform;
    cmd.instance_data.tint = tint;
    m_cmds.push_back(cmd);
  }
}

void RenderPass::draw_cube_wires(const vec3& pos, const vec3& size, const vec3& color)
{
  auto transform = get_transform_(pos, size, 0.0f);
  RenderCmd cmd = {};
  cmd.mesh = m_render_data.cube_wires;
  cmd.submesh_idx = 0;
  cmd.material = AssetManager::instance().get(m_render_data.cube_wires).submeshes[0].material;
  cmd.instance_data.transform = transform;
  cmd.instance_data.tint = color;
  m_cmds.push_back(cmd);
}

void RenderPass::draw_ring(const vec3& pos, f32 radius, const vec3& color)
{
  auto diameter = 2.0f * radius;
  auto transform = get_transform_(pos, {diameter, 1.0f, diameter}, 0.0f);
  RenderCmd cmd = {};
  cmd.mesh = m_render_data.ring;
  cmd.submesh_idx = 0;
  cmd.material = AssetManager::instance().get(m_render_data.ring).submeshes[0].material;
  cmd.instance_data.transform = transform;
  cmd.instance_data.tint = color;
  m_cmds.push_back(cmd);
}

void RenderPass::draw_line(const vec3& pos, f32 length, f32 rotation, const vec3& color)
{
  auto transform = get_transform_(pos, {length, 1.0f, length}, rotation);
  RenderCmd cmd = {};
  cmd.mesh = m_render_data.line;
  cmd.submesh_idx = 0;
  cmd.material = AssetManager::instance().get(m_render_data.line).submeshes[0].material;
  cmd.instance_data.transform = transform;
  cmd.instance_data.tint = color;
  m_cmds.push_back(cmd);
}

void RenderPass::set_light(const vec3& pos, const vec3& color)
{
  m_light.pos = pos;
  m_light.color = color;
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
  auto& assets = AssetManager::instance();
  glEnable(GL_DEPTH_TEST);

  glGenBuffers(1, &m_render_data.camera_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, m_render_data.camera_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Camera), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_CAMERA, m_render_data.camera_ubo);

  glGenBuffers(1, &m_render_data.lights_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, m_render_data.lights_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Light), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_LIGHTS, m_render_data.lights_ubo);

  glGenBuffers(1, &m_render_data.instance_data_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_render_data.instance_data_buffer);
  glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(InstanceData), nullptr, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_render_data.default_shader = assets.set({"shaders/shader.vert", "shaders/default.frag"});
  m_render_data.lighting_shader = assets.set({"shaders/shader.vert", "shaders/lighting.frag"});

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

  assets.bind_render_data(m_render_data);
}

void Renderer::create_framebuffer(TextureHandle texture)
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
  m_render_data.framebuffers.insert_or_assign(texture, id);
}

RenderPass Renderer::begin_pass(const Camera& camera, const vec3& ambient_color)
{
  return {m_render_data, camera, ambient_color};
}
