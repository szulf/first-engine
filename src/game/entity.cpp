#include "entity.h"

#include <filesystem>
#include <fstream>

#include "parser.h"

vec2 bounding_box_from_mesh(MeshHandle handle)
{
  vec3 max_corner = {std::numeric_limits<f32>::min(), 0, std::numeric_limits<f32>::min()};
  vec3 min_corner = {std::numeric_limits<f32>::max(), 0, std::numeric_limits<f32>::max()};
  const auto& mesh = asset_get(g_assets, handle);

  for (usize vertex_idx = 0; vertex_idx < mesh.vertices.size(); ++vertex_idx)
  {
    auto& vertex = mesh.vertices[vertex_idx];
    max_corner.x = std::max(max_corner.x, vertex.pos.x);
    min_corner.x = std::min(min_corner.x, vertex.pos.x);
    max_corner.z = std::max(max_corner.z, vertex.pos.z);
    min_corner.z = std::min(min_corner.z, vertex.pos.z);
  }
  return {max_corner.x - min_corner.x, max_corner.z - min_corner.z};
}

static vec3 gfmt_parse_vec3(Parser_Pos& pos)
{
  vec3 out{};
  parser_expect_and_skip(pos, '(');
  out.x = parser_number_f32(pos);
  parser_expect_and_skip(pos, ',');
  out.y = parser_number_f32(pos);
  parser_expect_and_skip(pos, ',');
  out.z = parser_number_f32(pos);
  parser_expect_and_skip(pos, ')');
  return out;
}

static vec2 gfmt_parse_vec2(Parser_Pos& pos)
{
  vec2 out{};
  parser_expect_and_skip(pos, '(');
  out.x = parser_number_f32(pos);
  parser_expect_and_skip(pos, ',');
  out.y = parser_number_f32(pos);
  parser_expect_and_skip(pos, ')');
  return out;
}

std::expected<Entity, std::string_view> entity_from_file(const std::filesystem::path& path)
{
  if (path.extension() != ".gent")
  {
    return std::unexpected{"Invalid filepath provided, expected a file with a '.gent' extension"};
  }
  std::ifstream file{path};
  if (file.fail())
  {
    return std::unexpected{"Failed to read entity file"};
  }
  Entity entity{};
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    Parser_Pos pos{.line = line};

    auto key = parser_word(pos);
    parser_expect_and_skip(pos, ':');
    if (key == "pos")
    {
      entity.pos = gfmt_parse_vec3(pos);
      entity.prev_pos = entity.pos;
    }
    else if (key == "controlled_by_player")
    {
      if (parser_boolean(pos))
      {
        entity.flags |= ENTITY_CONTROLLED_BY_PLAYER;
      }
    }
    else if (key == "rotation")
    {
      entity.rotation = parser_number_f32(pos);
      entity.prev_rotation = entity.rotation;
    }
    else if (key == "target_rotation")
    {
      entity.target_rotation = parser_number_f32(pos);
    }
    else if (key == "velocity")
    {
      entity.velocity = gfmt_parse_vec3(pos);
    }
    else if (key == "collidable")
    {
      if (parser_boolean(pos))
      {
        entity.flags |= ENTITY_COLLIDABLE;
      }
    }
    else if (key == "dynamically_calculated_bounding_box")
    {
      if (parser_boolean(pos))
      {
        entity.flags |= ENTITY_DYNAMIC_BOUNDING_BOX;
      }
    }
    else if (key == "bounding_box")
    {
      entity.bounding_box = gfmt_parse_vec2(pos);
    }
    else if (key == "visible")
    {
      if (parser_boolean(pos))
      {
        entity.flags |= ENTITY_VISIBLE;
      }
    }
    else if (key == "mesh")
    {
      entity.mesh_path = parser_word(pos);
      entity.mesh = load_obj(g_assets, entity.mesh_path);
    }
    else if (key == "toggleable")
    {
      if (parser_boolean(pos))
      {
        entity.flags |= ENTITY_TOGGLEABLE;
      }
    }
    else if (key == "interactable_radius")
    {
      entity.interactable_radius = parser_number_f32(pos);
    }
    else if (key == "emits_light")
    {
      if (parser_boolean(pos))
      {
        entity.flags |= ENTITY_EMITS_LIGHT;
      }
    }
    else if (key == "light_height_offset")
    {
      entity.light_height_offset = parser_number_f32(pos);
    }
    else if (key == "light_color")
    {
      entity.light_color = gfmt_parse_vec3(pos);
    }
    else if (key == "tint")
    {
      entity.tint = gfmt_parse_vec3(pos);
    }
    else if (key == "name")
    {
      entity.name = parser_word(pos);
    }
    else
    {
      return std::unexpected{"Invalid key in entity file"};
    }
  }
  if (entity.flags & ENTITY_DYNAMIC_BOUNDING_BOX)
  {
    entity.bounding_box = bounding_box_from_mesh(entity.mesh);
  }
  return {entity};
}

// NOTE: not combined with ui bounds checking,
// because the plan is to change the entity bounds checking algorithm in the future
bool entities_collide(const Entity& ea, const Entity& eb)
{
  auto& ax = ea.pos.x;
  auto& az = ea.pos.z;
  auto& bx = eb.pos.x;
  auto& bz = eb.pos.z;

  return ax - (ea.bounding_box.x / 2.0f) < bx + (eb.bounding_box.x / 2.0f) &&
         ax + (ea.bounding_box.x / 2.0f) > bx - (eb.bounding_box.x / 2.0f) &&
         az - (ea.bounding_box.y / 2.0f) < bz + (eb.bounding_box.y / 2.0f) &&
         az + (ea.bounding_box.y / 2.0f) > bz - (eb.bounding_box.y / 2.0f);
}

vec3 entity_render_pos(const Entity& entity, f32 t)
{
  return entity.pos * t + entity.prev_pos * (1.0f - t);
}

f32 entity_render_rotation(const Entity& entity, f32 t)
{
  // NOTE: checking if they have opposite signs
  if (entity.rotation * entity.prev_rotation < 0)
  {
    return entity.prev_rotation;
  }
  return entity.rotation * t + entity.prev_rotation * (1.0f - t);
}

std::expected<Scene, std::string_view> scene_from_file(const std::filesystem::path& path)
{
  if (path.extension() != ".gscn")
  {
    return std::unexpected{"Invalid filepath provided, expected a file with a '.gscn' extension"};
  }
  std::ifstream file{path};
  if (file.fail())
  {
    return std::unexpected{"Failed to read scene file"};
  }
  Scene scene{};
  std::string line{};
  std::unordered_map<std::string, Entity> entity_cache{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    Parser_Pos pos{.line = line};

    auto key = std::string{parser_word(pos)};
    parser_expect_and_skip(pos, ':');
    if (key == "ambient_color")
    {
      scene.ambient_color = gfmt_parse_vec3(pos);
      continue;
    }

    if (entity_cache.contains(key))
    {
      scene.entities.push_back(entity_cache[key]);
    }
    else
    {
      auto entity = entity_from_file((path.parent_path() / key).replace_extension(".gent"));
      if (!entity)
      {
        return std::unexpected{entity.error()};
      }
      entity_cache.insert_or_assign(key, *entity);
      scene.entities.push_back(*entity);
    }
    auto& entity = scene.entities[scene.entities.size() - 1];
    entity.prev_pos = entity.pos = gfmt_parse_vec3(pos);
  }

  return scene;
}
