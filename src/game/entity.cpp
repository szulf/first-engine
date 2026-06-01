#include "entity.h"

#include <filesystem>
#include <fstream>
#include <cmath>

#include "parser.h"
#include "assets.h"
#include "sound.h"

std::array<vec2, ENTITY_TYPE_COUNT> ENTITY_BOUNDING_BOX{};

std::string_view entity_type_to_string(EntityType type)
{
  switch (type)
  {
    case ENTITY_PLAYER:
      return "player";
    case ENTITY_BLOCK:
      return "block";
    case ENTITY_LIGHT_BULB:
      return "light_bulb";
    case ENTITY_CONVEYOR:
      return "conveyor";
    case ENTITY_STORAGE:
      return "storage";
    case ENTITY_TYPE_COUNT:
    default:
      ASSERT(false, "Invalid entity type");
  }
}

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
  else if (str == "conveyor")
  {
    return {ENTITY_CONVEYOR};
  }
  else if (str == "storage")
  {
    return {ENTITY_STORAGE};
  }
  return std::unexpected{"Invalid entity type string"};
}

Entity entity_new(EntityType type, AssetStore& assets)
{
  Entity entity{};
  std::memset(&entity, 0, sizeof(Entity));
  entity.type = type;
  if (entity.type == ENTITY_LIGHT_BULB)
  {
    entity.tint = EntityLightBulb::OFF_TINT;
  }
  entity.mesh = load_obj(assets, ENTITY_MESH_PATH[type]);
  if (ENTITY_BOUNDING_BOX[type] == vec2{})
  {
    ENTITY_BOUNDING_BOX[type] = bounding_box_from_mesh(entity.mesh, assets);
  }
  return entity;
}

// NOTE: not combined with ui bounds checking,
// because the plan is to change the entity bounds checking algorithm in the future
bool entities_collide(const Entity& ea, const Entity& eb)
{
  auto& ax = ea.pos.x;
  auto& az = ea.pos.z;
  auto& bx = eb.pos.x;
  auto& bz = eb.pos.z;

  return ax - (ENTITY_BOUNDING_BOX[ea.type].x / 2.0f) <
           bx + (ENTITY_BOUNDING_BOX[eb.type].x / 2.0f) &&
         ax + (ENTITY_BOUNDING_BOX[ea.type].x / 2.0f) >
           bx - (ENTITY_BOUNDING_BOX[eb.type].x / 2.0f) &&
         az - (ENTITY_BOUNDING_BOX[ea.type].y / 2.0f) <
           bz + (ENTITY_BOUNDING_BOX[eb.type].y / 2.0f) &&
         az + (ENTITY_BOUNDING_BOX[ea.type].y / 2.0f) >
           bz - (ENTITY_BOUNDING_BOX[eb.type].y / 2.0f);
}

vec3 entity_render_pos(const Entity& entity, f32 t)
{
  return entity.pos * t + entity.prev_pos * (1.0f - t);
}

f32 entity_render_rotation(const Entity& entity, f32 t)
{
  switch (entity.type)
  {
    case ENTITY_PLAYER:
      // NOTE: checking if they have opposite signs
      if (entity.player.rotation * entity.player.prev_rotation < 0)
      {
        return entity.player.prev_rotation;
      }
      return entity.player.rotation * t + entity.player.prev_rotation * (1.0f - t);
    case ENTITY_CONVEYOR:
      return entity.conveyor.rotation;
    case ENTITY_STORAGE:
      return entity.storage.rotation;
    case ENTITY_BLOCK:
    case ENTITY_LIGHT_BULB:
    case ENTITY_TYPE_COUNT:
    default:
      return 0;
  }
}

static vec3 gscn_parse_vec3(Parser_Pos& pos)
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

enum SceneLoadingMode
{
  SCENE_LOADING_GLOBAL,
  SCENE_LOADING_ENTITY,
};

std::expected<Scene, std::string_view>
load_scene(const std::filesystem::path& path, AssetStore& assets)
{
  std::ifstream file{path};
  if (file.fail())
  {
    return std::unexpected{"Failed to open scene file for reading"};
  }
  Scene scene{};
  SceneLoadingMode mode = SCENE_LOADING_GLOBAL;
  Entity entity{};
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    Parser_Pos pos{.line = line};

    switch (mode)
    {
      case SCENE_LOADING_GLOBAL:
      {
        auto key = parser_word(pos);
        parser_expect_and_skip(pos, ':');
        if (key == "ambient_color")
        {
          scene.ambient_color = gscn_parse_vec3(pos);
          continue;
        }
        auto entity_type = entity_type_from_string(key);
        if (!entity_type)
        {
          return std::unexpected{entity_type.error()};
        }
        entity = entity_new(*entity_type, assets);
        mode = SCENE_LOADING_ENTITY;
        parser_expect_and_skip(pos, '{');
      }
      break;
      case SCENE_LOADING_ENTITY:
      {
        auto key = parser_word(pos);
        if (key == "}")
        {
          scene.entities.push_back(entity);
          mode = SCENE_LOADING_GLOBAL;
          entity = {};
        }
        else
        {
          parser_expect_and_skip(pos, ':');
          if (key == "pos")
          {
            entity.pos = gscn_parse_vec3(pos);
          }
          else if (key == "tint")
          {
            entity.tint = gscn_parse_vec3(pos);
          }
          else
          {
            switch (entity.type)
            {
              case ENTITY_PLAYER:
                if (key == "rotation")
                {
                  entity.player.rotation = parser_number_f32(pos);
                  continue;
                }
                else if (key == "target_rotation")
                {
                  entity.player.target_rotation = parser_number_f32(pos);
                  continue;
                }
                else if (key == "velocity")
                {
                  entity.player.velocity = gscn_parse_vec3(pos);
                  continue;
                }
                break;
              case ENTITY_BLOCK:
                break;
              case ENTITY_LIGHT_BULB:
                if (key == "on")
                {
                  entity.light_bulb.on = parser_boolean(pos);
                  continue;
                }
                else if (key == "hovered")
                {
                  entity.light_bulb.hovered = parser_boolean(pos);
                  continue;
                }
                break;
              case ENTITY_CONVEYOR:
                if (key == "rotation")
                {
                  entity.conveyor.rotation = parser_number_f32(pos);
                  continue;
                }
                break;
              case ENTITY_STORAGE:
                if (key == "rotation")
                {
                  entity.storage.rotation = parser_number_f32(pos);
                  continue;
                }
                break;
              case ENTITY_TYPE_COUNT:
              default:
                break;
            }
            return std::unexpected{"Invalid key in scene file"};
          }
        }
      }
      break;
    }
  }

  return scene;
}

std::expected<void, std::string_view>
save_scene(const Scene& scene, const std::filesystem::path& path)
{
  std::ofstream file{path};
  if (file.fail())
  {
    return std::unexpected{"Failed to open scene file for writing"};
  }

  std::println(
    file,
    "ambient_color : ({}, {}, {})\n",
    scene.ambient_color.x,
    scene.ambient_color.y,
    scene.ambient_color.z
  );

  // TODO: should this maybe be something like Entities : { ... }
  for (usize entity_idx = 0; entity_idx < scene.entities.size(); ++entity_idx)
  {
    auto& entity = scene.entities[entity_idx];
    std::println(file, "{} : {{", entity_type_to_string(entity.type));
    defer(std::println(file, "}}\n"));

    std::println(file, "  pos : ({}, {}, {})", entity.pos.x, entity.pos.y, entity.pos.z);
    std::println(file, "  tint : ({}, {}, {})", entity.tint.x, entity.tint.y, entity.tint.z);

    switch (entity.type)
    {
      case ENTITY_PLAYER:
        std::println(file, "  rotation : {}", entity.player.rotation);
        std::println(file, "  target_rotation : {}", entity.player.target_rotation);
        std::println(
          file,
          "  velocity : ({}, {}, {})",
          entity.player.velocity.x,
          entity.player.velocity.y,
          entity.player.velocity.z
        );
        break;
      case ENTITY_BLOCK:
        break;
      case ENTITY_LIGHT_BULB:
        std::println(file, "  on : {}", entity.light_bulb.on);
        std::println(file, "  hovered : {}", entity.light_bulb.hovered);
        break;
      case ENTITY_CONVEYOR:
        std::println(file, "  rotation : {}", entity.conveyor.rotation);
        break;
      case ENTITY_STORAGE:
        std::println(file, "  rotation : {}", entity.storage.rotation);
        break;
      case ENTITY_TYPE_COUNT:
      default:
        break;
    }
  }

  return {};
}
