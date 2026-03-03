#include "entity.h"

#include <filesystem>
#include <fstream>

#include "parser.h"

vec2 bounding_box_from_mesh(MeshHandle handle)
{
  vec3 max_corner = {std::numeric_limits<f32>::min(), 0, std::numeric_limits<f32>::min()};
  vec3 min_corner = {std::numeric_limits<f32>::max(), 0, std::numeric_limits<f32>::max()};
  const auto& mesh = AssetManager::instance().get(handle);

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

static vec3 gfmt_parse_vec3(parser::Pos& pos)
{
  vec3 out{};
  parser::expect_and_skip(pos, '(');
  out.x = parser::number_f32(pos);
  parser::expect_and_skip(pos, ',');
  out.y = parser::number_f32(pos);
  parser::expect_and_skip(pos, ',');
  out.z = parser::number_f32(pos);
  parser::expect_and_skip(pos, ')');
  return out;
}

static vec2 gfmt_parse_vec2(parser::Pos& pos)
{
  vec2 out{};
  parser::expect_and_skip(pos, '(');
  out.x = parser::number_f32(pos);
  parser::expect_and_skip(pos, ',');
  out.y = parser::number_f32(pos);
  parser::expect_and_skip(pos, ')');
  return out;
}

Entity::Entity(const std::filesystem::path& path)
{
  ASSERT(
    path.extension() == ".gent",
    "[GENT] Invalid filepath provided. (path: {}).",
    path.string()
  );
  std::ifstream file{path};
  ASSERT(!file.fail(), "[GENT] File reading error. (path: {}).", path.string());
  std::string line{};
  bool calculate_bounding_box = false;
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    parser::Pos ppos{.line = line};

    auto key = parser::word(ppos);
    parser::expect_and_skip(ppos, ':');
    if (key == "pos")
    {
      pos = gfmt_parse_vec3(ppos);
      rendered_pos = prev_pos = pos;
    }
    else if (key == "controlled_by_player")
    {
      if (parser::boolean(ppos))
      {
        flags |= CONTROLLED_BY_PLAYER;
      }
    }
    else if (key == "rotation")
    {
      rotation = parser::number_f32(ppos);
      rendered_rotation = prev_rotation = rotation;
    }
    else if (key == "target_rotation")
    {
      target_rotation = parser::number_f32(ppos);
    }
    else if (key == "velocity")
    {
      velocity = gfmt_parse_vec3(ppos);
    }
    else if (key == "collidable")
    {
      if (parser::boolean(ppos))
      {
        flags |= COLLIDABLE;
      }
    }
    else if (key == "dynamically_calculated_bounding_box")
    {
      if (parser::boolean(ppos))
      {
        flags |= DYNAMIC_BOUNDING_BOX;
        calculate_bounding_box = true;
      }
    }
    else if (key == "bounding_box")
    {
      bounding_box = gfmt_parse_vec2(ppos);
    }
    else if (key == "renderable")
    {
      if (parser::boolean(ppos))
      {
        flags |= RENDERABLE;
      }
    }
    else if (key == "mesh")
    {
      mesh_path = parser::word(ppos);
      mesh = AssetManager::instance().load_obj(mesh_path);
    }
    else if (key == "interactable")
    {
      if (parser::boolean(ppos))
      {
        flags |= INTERACTABLE;
      }
    }
    else if (key == "interactable_radius")
    {
      interactable_radius = parser::number_f32(ppos);
    }
    else if (key == "emits_light")
    {
      if (parser::boolean(ppos))
      {
        flags |= EMITS_LIGHT;
      }
    }
    else if (key == "light_height_offset")
    {
      light_height_offset = parser::number_f32(ppos);
    }
    else if (key == "light_color")
    {
      light_color = gfmt_parse_vec3(ppos);
    }
    else if (key == "tint")
    {
      tint = gfmt_parse_vec3(ppos);
    }
    else if (key == "name")
    {
      name = parser::word(ppos);
    }
    else
    {
      ASSERT(false, "Invalid key. ({}).", key);
    }
  }
  if (calculate_bounding_box)
  {
    bounding_box = bounding_box_from_mesh(mesh);
  }
}

Scene::Scene(const std::filesystem::path& path)
{
  ASSERT(
    path.extension() == ".gscn",
    "[GSCN] Invalid filepath provided. (path: {}).",
    path.string()
  );
  std::ifstream file{path};
  ASSERT(!file.fail(), "[GSCN] File reading error. (path: {}).", path.string());
  std::string line{};
  std::unordered_map<std::string, Entity> entity_cache{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    parser::Pos pos{.line = line};

    auto key = std::string{parser::word(pos)};
    parser::expect_and_skip(pos, ':');
    if (key == "ambient_color")
    {
      ambient_color = gfmt_parse_vec3(pos);
      continue;
    }

    if (entity_cache.contains(key))
    {
      entities.push_back(entity_cache[key]);
    }
    else
    {
      Entity entity{(path.parent_path() / key).concat(".gent")};
      entity_cache.insert_or_assign(key, entity);
      entities.push_back(entity);
    }
    auto& ent = entities[entities.size() - 1];
    ent.prev_pos = ent.rendered_pos = ent.pos = gfmt_parse_vec3(pos);
  }
}
