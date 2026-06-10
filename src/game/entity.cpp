#include "entity.h"

#include <filesystem>
#include <fstream>
#include <cmath>

#include "base/errors.h"
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
    case ENTITY_ITEM:
      return "item";
    case ENTITY_TYPE_COUNT:
    default:
      ASSERT(false, "Invalid entity type");
  }
}

std::expected<EntityType, Error> entity_type_from_string(std::string_view str)
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
  else if (str == "item")
  {
    return {ENTITY_ITEM};
  }
  return std::unexpected{ERROR("Invalid entity type string: {}", str)};
}

ItemType item_type_from_entity_type(EntityType entity_type)
{
  for (usize item_type_idx = 0; item_type_idx < ITEM_TYPE_COUNT; ++item_type_idx)
  {
    if (entity_type == ENTITY_TYPE_FROM_ITEM_TYPE[(ItemType) item_type_idx])
    {
      return (ItemType) item_type_idx;
    }
  }
  // TODO: this probably shouldnt be an assert
  ASSERT(false, "Item doesnt exist for this entity type");
}

Entity entity_new(EntityType type, AssetStore& assets)
{
  Entity entity{};
  std::memset(&entity, 0, sizeof(Entity));
  entity.type = type;
  entity.scale = {1, 1, 1};
  if (entity.type == ENTITY_LIGHT_BULB)
  {
    entity.tint = EntityLightBulb::OFF_TINT;
  }
  else
  {
    entity.tint = {1, 1, 1};
  }
  // NOTE: ENTITY_ITEM mesh set in entity_new_item
  if (entity.type != ENTITY_ITEM)
  {
    entity.mesh = load_obj(assets, ENTITY_MESH_PATH[type]);
    if (ENTITY_BOUNDING_BOX[type] == vec2{})
    {
      ENTITY_BOUNDING_BOX[type] = bounding_box_from_mesh(entity.mesh, assets);
    }
  }
  return entity;
}

Entity entity_new_item(const ItemSlot& slot, AssetStore& assets)
{
  auto entity = entity_new(ENTITY_ITEM, assets);
  entity.scale = {EntityItem::SCALE, EntityItem::SCALE, EntityItem::SCALE};
  entity.item.slot = slot;
  entity.mesh = load_obj(assets, ENTITY_MESH_PATH[ENTITY_TYPE_FROM_ITEM_TYPE[slot.type]]);
  ENTITY_BOUNDING_BOX[ENTITY_ITEM] = {EntityItem::SCALE, EntityItem::SCALE};
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
    {
      f32 delta = wrap_to_neg_pi_to_pi(entity.player.rotation - entity.player.prev_rotation);
      return entity.player.prev_rotation + delta * t;
    }
    case ENTITY_CONVEYOR:
      return entity.conveyor.rotation;
    case ENTITY_STORAGE:
      return entity.storage.rotation;
    case ENTITY_ITEM:
      return entity.item.rotation;
    case ENTITY_BLOCK:
    case ENTITY_LIGHT_BULB:
    case ENTITY_TYPE_COUNT:
    default:
      return 0;
  }
}

static std::expected<vec3, Error> gscn_parse_vec3(Parser_Pos& pos)
{
  vec3 out{};
  TRY(parser_expect_and_skip(pos, '('));
  TRY_ASSIGN(out.x, parser_number_f32(pos));
  TRY(parser_expect_and_skip(pos, ','));
  TRY_ASSIGN(out.y, parser_number_f32(pos));
  TRY(parser_expect_and_skip(pos, ','));
  TRY_ASSIGN(out.z, parser_number_f32(pos));
  TRY(parser_expect_and_skip(pos, ')'));
  return {out};
}

static std::expected<ItemSlot, Error> gscn_parse_item_slot(Parser_Pos& pos)
{
  ItemSlot slot{};
  parser_skip_whitespace(pos);
  TRY(parser_expect_and_skip(pos, "ItemSlot{"));
  {
    TRY(parser_expect_and_skip(pos, "type"));
    TRY(parser_expect_and_skip(pos, ':'));
    TRY_ASSIGN(slot.type, item_type_from_string(parser_word(pos)));
  }
  TRY(parser_expect_and_skip(pos, ';'));
  {
    TRY(parser_expect_and_skip(pos, "count"));
    TRY(parser_expect_and_skip(pos, ':'));
    TRY_ASSIGN(slot.count, parser_number_u32(pos));
  }
  TRY(parser_expect_and_skip(pos, '}'));
  return {slot};
}

static std::expected<void, Error>
gscn_parse_inventory(Parser_Pos& pos, std::istream& is, ItemSlot* inventory, usize inventory_size)
{
  std::string line{};
  TRY(parser_expect_and_skip(pos, '['));
  for (usize slot_idx = 0; slot_idx < inventory_size; ++slot_idx)
  {
    std::getline(is, line);
    pos = {.line = line};
    TRY_ASSIGN(inventory[slot_idx], gscn_parse_item_slot(pos));
  }
  std::getline(is, line);
  pos = {.line = line};
  parser_skip_whitespace(pos);
  TRY(parser_expect_and_skip(pos, ']'));
  return {};
}

enum SceneLoadingMode
{
  SCENE_LOADING_GLOBAL,
  SCENE_LOADING_ENTITY,
};

std::expected<Scene, Error> load_scene(const std::filesystem::path& path, AssetStore& assets)
{
  Scene scene{};
  std::ifstream file{path};
  if (file.fail())
  {
    return std::unexpected{ERROR("Failed to open scene file at path: {}", path.string())};
  }
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
        TRY(parser_expect_and_skip(pos, ':'));
        if (key == "ambient_color")
        {
          TRY_ASSIGN(scene.ambient_color, gscn_parse_vec3(pos));
          continue;
        }
        EntityType entity_type{};
        TRY_ASSIGN(entity_type, entity_type_from_string(key));
        entity = entity_new(entity_type, assets);
        mode = SCENE_LOADING_ENTITY;
        TRY(parser_expect_and_skip(pos, '{'));
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
          TRY(parser_expect_and_skip(pos, ':'));
          if (key == "pos")
          {
            TRY_ASSIGN(entity.pos, gscn_parse_vec3(pos));
          }
          else if (key == "tint")
          {
            TRY_ASSIGN(entity.tint, gscn_parse_vec3(pos));
          }
          else
          {
            switch (entity.type)
            {
              case ENTITY_PLAYER:
                if (key == "rotation")
                {
                  TRY_ASSIGN(entity.player.rotation, parser_number_f32(pos));
                  continue;
                }
                else if (key == "target_rotation")
                {
                  TRY_ASSIGN(entity.player.target_rotation, parser_number_f32(pos));
                  continue;
                }
                else if (key == "velocity")
                {
                  TRY_ASSIGN(entity.player.velocity, gscn_parse_vec3(pos));
                  continue;
                }
                else if (key == "selected_hotbar_slot")
                {
                  TRY_ASSIGN_CAST(entity.player.selected_hotbar_slot, u8, parser_number_u32(pos));
                  continue;
                }
                else if (key == "is_inventory_open")
                {
                  TRY_ASSIGN(entity.player.is_inventory_open, parser_boolean(pos));
                  continue;
                }
                else if (key == "hand")
                {
                  TRY_ASSIGN(entity.player.hand, gscn_parse_item_slot(pos));
                  continue;
                }
                else if (key == "inventory")
                {
                  TRY(gscn_parse_inventory(
                    pos,
                    file,
                    entity.player.inventory,
                    EntityPlayer::INVENTORY_SIZE
                  ));
                  continue;
                }
                break;
              case ENTITY_BLOCK:
                break;
              case ENTITY_LIGHT_BULB:
                if (key == "on")
                {
                  TRY_ASSIGN(entity.light_bulb.on, parser_boolean(pos));
                  continue;
                }
                break;
              case ENTITY_CONVEYOR:
                if (key == "rotation")
                {
                  TRY_ASSIGN(entity.conveyor.rotation, parser_number_f32(pos));
                  continue;
                }
                break;
              case ENTITY_STORAGE:
                if (key == "rotation")
                {
                  TRY_ASSIGN(entity.storage.rotation, parser_number_f32(pos));
                  continue;
                }
                else if (key == "inventory")
                {
                  TRY(gscn_parse_inventory(
                    pos,
                    file,
                    entity.storage.inventory,
                    EntityStorage::INVENTORY_SIZE
                  ));
                  continue;
                }
                else if (key == "is_inventory_open")
                {
                  TRY_ASSIGN(entity.storage.is_inventory_open, parser_boolean(pos));
                  continue;
                }
                else if (key == "scroll_value")
                {
                  TRY_ASSIGN(entity.storage.scroll_value, parser_number_i32(pos));
                  continue;
                }
                break;
              case ENTITY_ITEM:
                if (key == "rotation")
                {
                  TRY_ASSIGN(entity.item.rotation, parser_number_f32(pos));
                  continue;
                }
                else if (key == "slot")
                {
                  // TODO: is there a better way to do this?
                  vec3 item_pos = entity.pos;
                  vec3 item_tint = entity.tint;
                  f32 item_rotation = entity.item.rotation;
                  ItemSlot item_slot{};
                  TRY_ASSIGN(item_slot, gscn_parse_item_slot(pos));
                  if (item_slot.count == 0)
                  {
                    return std::unexpected{ERROR("Item entities cannot have 0 item count")};
                  }
                  entity = entity_new_item(item_slot, assets);
                  entity.pos = item_pos;
                  entity.tint = item_tint;
                  entity.item.rotation = item_rotation;
                  continue;
                }
                break;
              case ENTITY_TYPE_COUNT:
                break;
            }
            return std::unexpected{ERROR("Invalid key in scene file: {}", key)};
          }
        }
      }
      break;
    }
  }
  return {scene};
}

static void gscn_write_item_slot(std::ostream& os, const ItemSlot& slot)
{
  std::println(os, "ItemSlot{{ type : {} ; count : {} }}", ITEM_TYPE_STRING[slot.type], slot.count);
}

// TODO: think about the indentation here
static void gscn_write_inventory(std::ostream& os, const ItemSlot* inventory, usize inventory_size)
{
  std::println(os, "  inventory : [");
  for (usize slot_idx = 0; slot_idx < inventory_size; ++slot_idx)
  {
    std::print(os, "    ");
    gscn_write_item_slot(os, inventory[slot_idx]);
  }
  std::println(os, "  ]");
}

std::expected<void, Error> save_scene(const Scene& scene, const std::filesystem::path& path)
{
  std::ofstream file{path};
  if (file.fail())
  {
    return std::unexpected{ERROR("Failed to open scene file at path: {}", path.string())};
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
        gscn_write_inventory(file, entity.player.inventory, EntityPlayer::INVENTORY_SIZE);
        std::println(file, "  selected_hotbar_slot : {}", entity.player.selected_hotbar_slot);
        std::println(file, "  is_inventory_open : {}", entity.player.is_inventory_open);
        std::print(file, "  hand : ");
        gscn_write_item_slot(file, entity.player.hand);
        break;
      case ENTITY_BLOCK:
        break;
      case ENTITY_LIGHT_BULB:
        std::println(file, "  on : {}", entity.light_bulb.on);
        break;
      case ENTITY_CONVEYOR:
        std::println(file, "  rotation : {}", entity.conveyor.rotation);
        break;
      case ENTITY_STORAGE:
        std::println(file, "  rotation : {}", entity.storage.rotation);
        gscn_write_inventory(file, entity.storage.inventory, EntityStorage::INVENTORY_SIZE);
        std::println(file, "  is_inventory_open : {}", entity.storage.is_inventory_open);
        std::println(file, "  scroll_value : {}", entity.storage.scroll_value);
        break;
      case ENTITY_ITEM:
        std::println(file, "  rotation : {}", entity.item.rotation);
        std::print(file, "  slot : ");
        gscn_write_item_slot(file, entity.item.slot);
        break;
      case ENTITY_TYPE_COUNT:
        break;
    }
  }
  return {};
}
