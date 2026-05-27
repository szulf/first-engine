#include "entity.h"

#include <filesystem>
#include <fstream>
#include <cmath>

#include "parser.h"
#include "assets.h"
#include "sound.h"

std::expected<EntityType, std::string_view> entity_type_from_string(std::string_view str)
{
  if (str == "player")
  {
    return {ENTITY_PLAYER};
  }
  else if (str == "block")
  {
    return {ENTITY_BLOCK};
  }
  else if (str == "light_bulb")
  {
    return {ENTITY_LIGHT_BULB};
  }
  return std::unexpected{"Invalid entity type string"};
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

static std::string_view mesh_path_from_entity_type(EntityType type)
{
  switch (type)
  {
    case ENTITY_PLAYER:
      return EntityPlayer::MESH_PATH;
    case ENTITY_BLOCK:
      return EntityBlock::MESH_PATH;
    case ENTITY_LIGHT_BULB:
      return EntityLightBulb::MESH_PATH;
  }
  ASSERT(false, "Invalid entity type");
}

std::expected<Scene, std::string_view>
scene_from_file(const std::filesystem::path& path, AssetStore& assets)
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

    auto entity_type = entity_type_from_string(key);
    if (!entity_type)
    {
      return std::unexpected{entity_type.error()};
    }
    Entity entity{};
    entity.type = *entity_type;
    if (entity.type == ENTITY_LIGHT_BULB)
    {
      entity.tint = EntityLightBulb::OFF_TINT;
    }
    entity.mesh = load_obj(assets, mesh_path_from_entity_type(entity.type));
    entity.bounding_box = bounding_box_from_mesh(entity.mesh, assets);
    entity.prev_pos = entity.pos = gfmt_parse_vec3(pos);
    scene.entities.push_back(entity);
  }

  return scene;
}
