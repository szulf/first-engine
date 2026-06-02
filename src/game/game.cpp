#include "game.h"

#include <cmath>
#include <fstream>
#include <string>

#include "base/base.h"
#include "base/errors.h"
#include "os/os.h"
#include "camera.h"
#include "assets.h"
#include "sound.h"
#include "renderer.h"
#include "entity.h"
#include "parser.h"
#include "ui.h"

std::expected<Action, std::string_view> string_to_action(std::string_view str)
{
  if (str == "move_front")
  {
    return {ACTION_MOVE_FRONT};
  }
  else if (str == "move_back")
  {
    return {ACTION_MOVE_BACK};
  }
  else if (str == "move_left")
  {
    return {ACTION_MOVE_LEFT};
  }
  else if (str == "move_right")
  {
    return {ACTION_MOVE_RIGHT};
  }
  else if (str == "interact")
  {
    return {ACTION_INTERACT};
  }
  else if (str == "slot1")
  {
    return {ACTION_SLOT_1};
  }
  else if (str == "slot2")
  {
    return {ACTION_SLOT_2};
  }
  else if (str == "slot3")
  {
    return {ACTION_SLOT_3};
  }
  else if (str == "save_scene")
  {
    return {ACTION_SAVE_SCENE};
  }
  else if (str == "load_scene")
  {
    return {ACTION_LOAD_SCENE};
  }
  else if (str == "rotate_entity_to_place")
  {
    return {ACTION_ROTATE_ENTITY_TO_PLACE};
  }
  else if (str == "camera_move_up")
  {
    return {ACTION_CAMERA_MOVE_UP};
  }
  else if (str == "camera_move_down")
  {
    return {ACTION_CAMERA_MOVE_DOWN};
  }
  else if (str == "toggle_debug_menu")
  {
    return {ACTION_TOGGLE_DEBUG_MENU};
  }
  else if (str == "toggle_noclip")
  {
    return {ACTION_TOGGLE_NOCLIP};
  }
  return std::unexpected{"Invalid action string"};
}

Keymap load_gkey(const std::filesystem::path& path)
{
  Keymap keymap = DEFAULT_KEYMAP;
  std::ifstream file{path};
  if (file.fail())
  {
    REPORT_ERROR("Failed to open keymap file");
    return keymap;
  }
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    Parser_Pos pos{.line = line};

    auto action_str = parser_word(pos);
    auto action = string_to_action(action_str);
    if (!action)
    {
      REPORT_ERROR("Invalid action");
      return keymap;
    }
    parser_expect_and_skip(pos, ':');
    auto key_str = parser_word(pos);
    auto key = os_string_to_key(key_str);
    if (!key)
    {
      REPORT_ERROR("Invalid key");
      return keymap;
    }
    keymap.map[*action] = *key;
  }
  return keymap;
}

static TextureHandle create_item_icon(ItemType item_type, AssetStore& assets)
{
  Camera icon_camera = {
    .type = CAMERA_TYPE_PERSPECTIVE,
    .pos = {0, 0.9f, 1.8f},
    .prev_pos = {0, 0.9f, 1.8f},
    .yaw = -90,
    .pitch = -30,
    .fov_type = FOV_TYPE_VERTICAL,
    .fov = -0.25f * std::numbers::pi_v<f32>,
    .near_plane = 0.1f,
    .far_plane = 5,
    .viewport = {256, 256},
  };
  camera_update_vectors(icon_camera);

  TextureHandle icon = asset_set(assets, texture_init(TEXTURE_2D, {256, 256}));
  render_create_framebuffer(icon, assets);
  Render_Pass pass{.camera = &icon_camera};
  render_pass_render_to(pass, icon);
  render_pass_override_shader(pass, g_render_data.default_shader);
  render_mesh(
    pass.cmds_3d,
    load_obj(assets, ENTITY_MESH_PATH[ENTITY_TYPE_FROM_ITEM_TYPE[item_type]]),
    {0, 0, 0},
    -0.25f * std::numbers::pi_v<f32>,
    {1, 1, 1},
    {1, 1, 1},
    assets
  );
  render_pass_finish(pass, assets);
  return icon;
}

GameData game_init(OS_Window& window, OS_Audio& audio)
{
  GameData game{};
  game.window = &window;
  game.sound_system = sound_system_init(audio);
  sound_system_start(game.sound_system);
  game.keymap = load_gkey("data/keymap.gkey");
  game.gameplay_camera = {
    .type = CAMERA_TYPE_PERSPECTIVE,
    .pos = {0, 12, 8},
    .prev_pos = {0, 12, 8},
    .yaw = -90,
    .pitch = -55,
    .fov_type = FOV_TYPE_VERTICAL,
    .fov = 0.25f * std::numbers::pi_v<f32>,
    .near_plane = 0.1f,
    .far_plane = 1000.0f,
    .viewport = window.dimensions,
  };
  camera_update_vectors(game.gameplay_camera);
  game.debug_camera = game.gameplay_camera;
  game.used_camera = &game.gameplay_camera;
  render_init(game.assets);
  {
    auto scene = load_scene("data/main.gscn", game.assets);
    ASSERT(scene, "Failed to load scene {}", scene.error());
    game.scene = *scene;
  }
  game.shadow_map = asset_set(game.assets, texture_init(TEXTURE_CUBEMAP, SHADOW_MAP_DIMENSIONS));
  {
    auto shader = shader_from_file(
      "shaders/shadow_depth.vert",
      "shaders/shadow_depth.frag",
      "shaders/shadow_depth.geom"
    );
    if (shader)
    {
      game.shadow_depth_shader = asset_set(game.assets, *shader);
    }
    else
    {
      REPORT_ERROR(shader.error());
    }
  }
  render_create_framebuffer(game.shadow_map, game.assets);
  // TODO: load this with FilterOption::NEAREST
  game.font_texture = load_texture(game.assets, "assets/font.png");
  // m_sound_system.play_looped(SoundHandle::TEST_MUSIC, 0.1f);

  for (usize item_type = 0; item_type < ITEM_TYPE_COUNT; ++item_type)
  {
    game.item_icons[item_type] = create_item_icon((ItemType) item_type, game.assets);
  }

  return game;
}

void game_deinit(GameData& game)
{
  sound_system_deinit(game.sound_system);
}

static bool in_player_interaction_radius(const Entity& player, const vec3& pos)
{
  f32 dist2_from_player = length2(pos - player.pos);
  return dist2_from_player < square(EntityPlayer::INTERACTION_RADIUS);
}

static bool
inventory_slot_ui(UI_Layout& layout, const ItemSlot& slot, bool selected, GameData& game)
{
  bool clicked_parent = false;
  bool clicked_child = false;
  ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked_parent});
  defer(ui_element_end(
    layout,
    {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
     .sizing = {ui_sizing_fixed(64), ui_sizing_fit()},
     .padding = {4, 4, 0, 0},
     .child_gap = 4,
     .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
     .bg_color = selected ? vec4{0.5f, 0.5f, 0.5f, 1} : vec4{0.3f, 0.3f, 0.3f, 1}}
  ));
  {
    if (slot.count != 0)
    {
      ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked_child});
      ui_element_end(
        layout,
        {.sizing = {ui_sizing_fixed(56), ui_sizing_fixed(56)}, .texture = game.item_icons[slot.type]
        }
      );
    }
    // NOTE: hack to get the proper height
    else
    {
      ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked_child});
      ui_element_end(
        layout,
        {.sizing = {ui_sizing_fixed(56), ui_sizing_fixed(56)}, .bg_color = {0, 0, 0, 0}}
      );
    }
    ui_text(layout, std::format("{}", slot.count), 1);
  }
  return clicked_parent || clicked_child;
}

static void handle_hand_slot_interaction(ItemSlot& hand, ItemSlot& slot)
{
  if (hand.count > 0 && hand.count < ITEMS_MAX_STACK_SIZE && slot.count < ITEMS_MAX_STACK_SIZE &&
      hand.type == slot.type)
  {
    if (slot.count + hand.count > ITEMS_MAX_STACK_SIZE)
    {
      hand.count = (slot.count + hand.count) - ITEMS_MAX_STACK_SIZE;
      slot.count = ITEMS_MAX_STACK_SIZE;
    }
    else
    {
      slot.count += hand.count;
      hand = {};
    }
  }
  else
  {
    std::swap(hand, slot);
  }
}

static bool
entity_exists_at_mouse(const vec3& mouse_pos, EntityType mouse_entity_type, const Scene& scene)
{
  for (usize i = 0; i < scene.entities.size(); ++i)
  {
    if (f32_equal(scene.entities[i].pos.y, 0) &&
        entities_collide(scene.entities[i], {.type = mouse_entity_type, .pos = mouse_pos}))
    {
      return true;
    }
  }
  return false;
}

static void drop_item(const ItemSlot& slot, const vec3& pos, Scene& scene, AssetStore& assets)
{
  auto dropped_entity = entity_new_item(slot, assets);
  dropped_entity.pos = pos;
  scene.entity_place_queue.push_back(dropped_entity);
}

void game_update_tick(GameData& game, f32 dt)
{
  ui_system_update(game.ui_system);
  game.gameplay_camera.viewport = game.debug_camera.viewport = game.window->dimensions;

  if (os_key_just_pressed(key_state_from_action(ACTION_TOGGLE_DEBUG_MENU, game)))
  {
    game.debug.menu.shown = !game.debug.menu.shown;
    game.debug.menu.drag = false;
    game.debug.error_list.drag = false;
  }
  if (os_key_just_pressed(key_state_from_action(ACTION_TOGGLE_NOCLIP, game)))
  {
    game.debug.noclip = !game.debug.noclip;
  }

  vec3 acceleration = {};
  if (key_state_from_action(ACTION_MOVE_FRONT, game).down)
  {
    acceleration.z += -1.0f;
  }
  if (key_state_from_action(ACTION_MOVE_BACK, game).down)
  {
    acceleration.z += 1.0f;
  }
  if (key_state_from_action(ACTION_MOVE_LEFT, game).down)
  {
    acceleration.x += -1.0f;
  }
  if (key_state_from_action(ACTION_MOVE_RIGHT, game).down)
  {
    acceleration.x += 1.0f;
  }
  acceleration = normalize(acceleration);

  // TODO: i really dont like this, maybe just store the player separately from the entities vector?
  // or just put the hotbar ui in players update loop?
  // NOTE: saving the player entity for easy access later in ui code
  // (need to use it to render hotbar/inventory)
  Entity* player{};
  // NOTE: using a special loop just for this, because this still needs to work even if noclip is on
  for (usize entity_idx = 0; entity_idx < game.scene.entities.size(); ++entity_idx)
  {
    if (game.scene.entities[entity_idx].type == ENTITY_PLAYER)
    {
      player = &game.scene.entities[entity_idx];
      break;
    }
  }

  if (game.debug.noclip)
  {
    game.used_camera = &game.debug_camera;
    os_hide_mouse_pointer();
    os_window_center_mouse_pointer(*game.window);

    if (key_state_from_action(ACTION_CAMERA_MOVE_UP, game).down)
    {
      acceleration.y += 1.0f;
    }
    if (key_state_from_action(ACTION_CAMERA_MOVE_DOWN, game).down)
    {
      acceleration.y += -1.0f;
    }

    auto forward = cross(CAMERA_WORLD_UP, game.debug_camera.right);
    camera_move(game.debug_camera, acceleration, forward, dt);
  }
  else
  {
    game.used_camera = &game.gameplay_camera;
    os_show_mouse_pointer();

    if (os_key_just_pressed(key_state_from_action(ACTION_SAVE_SCENE, game)))
    {
      auto res = save_scene(game.scene, "data/main.gscn");
      if (!res)
      {
        REPORT_ERROR("Failed to save scene");
      }
    }
    // NOTE: this has to be before any of the updates to the scene
    if (os_key_just_pressed(key_state_from_action(ACTION_LOAD_SCENE, game)))
    {
      auto scene = load_scene("data/main.gscn", game.assets);
      if (!scene)
      {
        REPORT_ERROR("Failed to load new scene");
      }
      game.scene = *scene;
    }

    // NOTE: calculate mouse position in world space
    vec3 mouse_world_pos{};
    {
      vec2 ndc = ((game.window->input.mouse_pos / game.window->dimensions) * 2) - vec2{1, 1};
      ndc.y = -ndc.y;
      vec4 ray_clip = {ndc.x, ndc.y, -1, 1};
      vec4 ray_view = inverse(camera_projection(game.gameplay_camera)) * ray_clip;
      ray_view.z = -1;
      ray_view.w = 0;
      vec4 ray_world = inverse(camera_view(game.gameplay_camera, 0)) * ray_view;
      vec3 ray = normalize(vec3{ray_world.x, ray_world.y, ray_world.z});
      f32 t = (0 - game.gameplay_camera.pos.y) / ray.y;
      mouse_world_pos = game.gameplay_camera.pos + t * ray;
      game.mouse_tile_pos.x = std::round(mouse_world_pos.x);
      game.mouse_tile_pos.z = std::round(mouse_world_pos.z);
    }

    // NOTE: actually place entities
    for (usize place_idx = 0; place_idx < game.scene.entity_place_queue.size(); ++place_idx)
    {
      game.scene.entities.push_back(game.scene.entity_place_queue[place_idx]);
    }
    game.scene.entity_place_queue.clear();

    // NOTE: actually remove entities
    for (usize remove_idx = 0; remove_idx < game.scene.entity_idx_remove_queue.size(); ++remove_idx)
    {
      game.scene.entities.erase(
        game.scene.entities.begin() + (isize) game.scene.entity_idx_remove_queue[remove_idx]
      );
    }
    game.scene.entity_idx_remove_queue.clear();

    // NOTE: entity update
    for (usize entity_idx = 0; entity_idx < game.scene.entities.size(); ++entity_idx)
    {
      auto& entity = game.scene.entities[entity_idx];
      entity.prev_pos = entity.pos;

      if (entity.type == ENTITY_PLAYER)
      {
        entity.player.prev_rotation = entity.player.rotation;
        game.mouse_in_player_interaction_radius =
          in_player_interaction_radius(entity, game.mouse_tile_pos);

        // NOTE: select hotbar slot
        for (usize slot_action = ACTION_SLOT_1; slot_action <= ACTION_SLOT_4; ++slot_action)
        {
          if (os_key_just_pressed(key_state_from_action((Action) slot_action, game)))
          {
            entity.player.selected_hotbar_slot = (u8) (slot_action - ACTION_SLOT_1);
          }
        }

        // NOTE: open inventory
        if (os_key_just_pressed(key_state_from_action(ACTION_OPEN_INVENTORY, game)))
        {
          entity.player.is_inventory_open = !entity.player.is_inventory_open;
        }
        if (entity.player.is_inventory_open &&
            os_key_just_pressed(game.window->input.keys[OS_KEY_ESCAPE]))
        {
          entity.player.is_inventory_open = false;
        }

        // TODO: do i want this only if game.mouse_in_player_interaction_radius?
        // NOTE: rotate entity to place
        if (os_key_just_pressed(key_state_from_action(ACTION_ROTATE_ENTITY_TO_PLACE, game)) &&
            ENTITY_ROTATABLE[player_selected_hotbar_slot_entity_type(entity)])
        {
          game.scene.place_rotation = (game.scene.place_rotation + 1) % 4;
        }

        // TODO: game.mouse_over_player_inventory here is from the last frame,
        // but the change i have in mind (moving ui to update_frame) would fix this
        // NOTE: queue entities to place
        if (game.window->input.rmb.down && game.mouse_in_player_interaction_radius &&
            player_selected_hotbar_slot(entity).count > 0 && !game.mouse_over_player_inventory &&
            entity.player.hand.count == 0)
        {
          if (!entity_exists_at_mouse(
                game.mouse_tile_pos,
                player_selected_hotbar_slot_entity_type(entity),
                game.scene
              ))
          {
            auto placed_entity =
              entity_new(player_selected_hotbar_slot_entity_type(entity), game.assets);
            placed_entity.pos = game.mouse_tile_pos;
            if (ENTITY_ROTATABLE[player_selected_hotbar_slot_entity_type(entity)])
            {
              switch (player_selected_hotbar_slot(entity).type)
              {
                case ITEM_CONVEYOR:
                  placed_entity.conveyor.rotation =
                    (f32) game.scene.place_rotation * (0.5f * std::numbers::pi_v<f32>);
                  break;
                case ITEM_STORAGE:
                  placed_entity.storage.rotation =
                    (f32) game.scene.place_rotation * (0.5f * std::numbers::pi_v<f32>);
                  break;
                case ITEM_BLOCK:
                case ITEM_LIGHT_BULB:
                case ITEM_TYPE_COUNT:
                default:
                  break;
              }
            }
            game.scene.entity_place_queue.push_back(placed_entity);
            --player_selected_hotbar_slot(entity).count;
          }
        }

        // TODO: game.mouse_over_player_inventory here is from the last frame,
        // but the change i have in mind (moving ui to update_frame) would fix this
        // NOTE: queue entities to remove
        if (os_key_just_pressed(game.window->input.lmb) &&
            game.mouse_in_player_interaction_radius && !game.mouse_over_player_inventory &&
            entity.player.hand.count == 0)
        {
          for (usize i = 0; i < game.scene.entities.size(); ++i)
          {
            auto& remove_entity = game.scene.entities[i];
            if (!ENTITY_UNBREAKABLE[remove_entity.type] && remove_entity.pos == game.mouse_tile_pos)
            {
              if (ENTITY_HAS_INVENTORY[remove_entity.type])
              {
                switch (remove_entity.type)
                {
                  case ENTITY_STORAGE:
                  {
                    for (usize slot_idx = 0; slot_idx < EntityStorage::INVENTORY_SIZE; ++slot_idx)
                    {
                      auto& slot = remove_entity.storage.inventory[slot_idx];
                      if (slot.count > 0)
                      {
                        drop_item(slot, remove_entity.pos, game.scene, game.assets);
                      }
                    }
                  }
                  break;

                  case ENTITY_PLAYER:
                  case ENTITY_BLOCK:
                  case ENTITY_LIGHT_BULB:
                  case ENTITY_CONVEYOR:
                  case ENTITY_ITEM:
                  case ENTITY_TYPE_COUNT:
                    break;
                }
              }
              drop_item(
                {.type = item_type_from_entity_type(remove_entity.type), .count = 1},
                remove_entity.pos,
                game.scene,
                game.assets
              );
              game.scene.entity_idx_remove_queue.push_back(i);
              break;
            }
          }
        }

        // NOTE: drop items out of hand
        if (os_key_just_pressed(game.window->input.lmb) &&
            game.mouse_in_player_interaction_radius && !game.mouse_over_player_inventory &&
            entity.player.hand.count > 0 &&
            !entity_exists_at_mouse(mouse_world_pos, ENTITY_ITEM, game.scene))
        {
          drop_item(entity.player.hand, mouse_world_pos, game.scene, game.assets);
          entity.player.hand = {};
        }

        // NOTE: player rotation
        {
          if (acceleration != vec3{0.0f, 0.0f, 0.0f})
          {
            auto rot = std::atan2(-acceleration.x, acceleration.z);
            entity.player.target_rotation = rot;
          }
          f32 direction =
            wrap_to_neg_pi_to_pi(entity.player.target_rotation - entity.player.rotation);
          entity.player.rotation += direction * EntityPlayer::ROTATION_SPEED * dt;
          entity.player.rotation = wrap_to_neg_pi_to_pi(entity.player.rotation);
        }

        // NOTE: movement and collisions
        // TODO: this is still a little wrong in some cases,
        // maybe use a better algorithm overall to improve this
        {
          static constexpr f32 FRICTION_COEFFICIENT = 0.35f;
          static constexpr f32 NORMAL_FORCE = EntityPlayer::MASS * G_F32;
          static constexpr f32 FRICTION_MAGNITUDE = FRICTION_COEFFICIENT * NORMAL_FORCE;

          acceleration *= EntityPlayer::MOVEMENT_SPEED;

          vec3 friction_dir = -normalize(entity.player.velocity);
          vec3 friction_force = friction_dir * FRICTION_MAGNITUDE;

          vec3 drag = -3.0f * entity.player.velocity;
          vec3 friction = (friction_force / EntityPlayer::MASS) + drag;

          acceleration += friction;
          auto new_pos = 0.5f * acceleration * (dt * dt) + entity.player.velocity * dt + entity.pos;
          entity.player.velocity = acceleration * dt + entity.player.velocity;

          vec3 collision_normal = {};
          bool collided = false;
          for (usize collidable_idx = 0; collidable_idx < game.scene.entities.size();
               ++collidable_idx)
          {
            auto& c = game.scene.entities[collidable_idx];
            if (c.type == ENTITY_PLAYER || !f32_equal(c.pos.y, 0))
            {
              continue;
            }

            vec3 rounded_pos = {std::round(new_pos.x), 0.0f, std::round(new_pos.z)};
            if ((c.pos.x > rounded_pos.x + 1.0f || c.pos.x < rounded_pos.x - 1.0f) ||
                (c.pos.z > rounded_pos.z + 1.0f || c.pos.z < rounded_pos.z - 1.0f))
            {
              continue;
            }

            Entity p = {};
            p.type = entity.type;
            p.pos = new_pos;

            if (!entities_collide(p, c))
            {
              continue;
            }

            if (c.type == ENTITY_ITEM)
            {
              for (usize slot_idx = 0; slot_idx < EntityPlayer::INVENTORY_SIZE; ++slot_idx)
              {
                auto& slot = entity.player.inventory[slot_idx];
                if (slot.count == 0)
                {
                  slot.type = c.item.slot.type;
                  slot.count = c.item.slot.count;
                  game.scene.entity_idx_remove_queue.push_back(collidable_idx);
                  break;
                }
                else if (slot.type == c.item.slot.type && slot.count != ITEMS_MAX_STACK_SIZE)
                {
                  if (slot.count + c.item.slot.count <= ITEMS_MAX_STACK_SIZE)
                  {
                    slot.count += c.item.slot.count;
                    game.scene.entity_idx_remove_queue.push_back(collidable_idx);
                    break;
                  }
                  else
                  {
                    c.item.slot.count = (slot.count + c.item.slot.count) - ITEMS_MAX_STACK_SIZE;
                    slot.count = ITEMS_MAX_STACK_SIZE;
                  }
                }
              }
              continue;
            }

            auto collidable_front = c.pos.z + (0.5f * ENTITY_BOUNDING_BOX[c.type].y);
            auto collidable_back = c.pos.z - (0.5f * ENTITY_BOUNDING_BOX[c.type].y);
            auto collidable_left = c.pos.x - (0.5f * ENTITY_BOUNDING_BOX[c.type].x);
            auto collidable_right = c.pos.x + (0.5f * ENTITY_BOUNDING_BOX[c.type].x);

            auto entity_front = p.pos.z + (0.5f * ENTITY_BOUNDING_BOX[p.type].y);
            auto entity_back = p.pos.z - (0.5f * ENTITY_BOUNDING_BOX[p.type].y);
            auto entity_left = p.pos.x - (0.5f * ENTITY_BOUNDING_BOX[p.type].x);
            auto entity_right = p.pos.x + (0.5f * ENTITY_BOUNDING_BOX[p.type].x);

            auto back_overlap = std::abs(entity_back - collidable_front);
            auto front_overlap = std::abs(entity_front - collidable_back);
            auto left_overlap = std::abs(entity_left - collidable_right);
            auto right_overlap = std::abs(entity_right - collidable_left);

            auto collision_overlap = std::min(
              std::min(std::min(back_overlap, front_overlap), left_overlap),
              right_overlap
            );

            if (f32_equal(collision_overlap, back_overlap))
            {
              collision_normal.z = -1.0f;
            }
            else if (f32_equal(collision_overlap, front_overlap))
            {
              collision_normal.z = 1.0f;
            }
            else if (f32_equal(collision_overlap, left_overlap))
            {
              collision_normal.x = 1.0f;
            }
            else if (f32_equal(collision_overlap, right_overlap))
            {
              collision_normal.x = -1.0f;
            }
            collided = true;
          }

          auto abs_collision_normal = abs(collision_normal);
          auto collision_normal_inverted = vec3{1.0f, 0.0f, 1.0f} - abs_collision_normal;
          entity.pos = (abs_collision_normal * entity.pos) + (new_pos * collision_normal_inverted);
          if (collided)
          {
            entity.player.velocity -=
              dot(entity.player.velocity, collision_normal) * collision_normal;
            sound_play_once(game.sound_system, SOUND_HANDLE_SINE, 0.1f);
          }
        }

        // TODO: can this be part of updates and not a loop in players update?
        // NOTE: interactions
        for (usize idx = 0; idx < game.scene.entities.size(); ++idx)
        {
          auto& interactee = game.scene.entities[idx];
          vec2 max_mouse_dist = {
            ENTITY_BOUNDING_BOX[interactee.type].x / 2.0f,
            ENTITY_BOUNDING_BOX[interactee.type].y / 2.0f
          };
          bool mouse_in_entity_range_x =
            std::abs(game.mouse_tile_pos.x - interactee.pos.x) < max_mouse_dist.x;
          bool mouse_in_entity_range_z =
            std::abs(game.mouse_tile_pos.z - interactee.pos.z) < max_mouse_dist.y;
          bool mouse_in_entity_range = mouse_in_entity_range_x && mouse_in_entity_range_z;
          switch (interactee.type)
          {
            case ENTITY_LIGHT_BULB:
            {
              interactee.light_bulb.hovered = false;
              if (in_player_interaction_radius(entity, interactee.pos) && mouse_in_entity_range &&
                  !game.mouse_over_player_inventory)
              {
                interactee.light_bulb.hovered = true;
              }

              if (interactee.light_bulb.hovered &&
                  os_key_just_pressed(key_state_from_action(ACTION_INTERACT, game)))
              {
                interactee.light_bulb.on = !interactee.light_bulb.on;
                interactee.tint =
                  interactee.light_bulb.on ? EntityLightBulb::ON_TINT : EntityLightBulb::OFF_TINT;
                sound_play_once(game.sound_system, SOUND_HANDLE_SHOTGUN, 0.1f);
                sound_stop_looped(game.sound_system, SOUND_HANDLE_TEST_MUSIC);
              }
            }
            break;

            case ENTITY_STORAGE:
            {
              // TODO: can i get this logic better?
              if (interactee.storage.is_inventory_open &&
                  !in_player_interaction_radius(entity, interactee.pos))
              {
                interactee.storage.is_inventory_open = false;
              }
              interactee.storage.hovered = in_player_interaction_radius(entity, interactee.pos) &&
                                           mouse_in_entity_range &&
                                           !game.mouse_over_player_inventory;
              if (!interactee.storage.is_inventory_open &&
                  in_player_interaction_radius(entity, interactee.pos) && mouse_in_entity_range &&
                  !game.mouse_over_player_inventory)
              {
                if (os_key_just_pressed(key_state_from_action(ACTION_INTERACT, game)))
                {
                  // TODO: this is horrible, but should be fine after the entity refactor
                  for (usize storage_idx = 0; storage_idx < game.scene.entities.size();
                       ++storage_idx)
                  {
                    if (game.scene.entities[storage_idx].type == ENTITY_STORAGE &&
                        game.scene.entities[storage_idx].storage.is_inventory_open)
                    {
                      game.scene.entities[storage_idx].storage.is_inventory_open = false;
                    }
                  }
                  interactee.storage.is_inventory_open = true;
                }
              }
              else
              {
                if ((os_key_just_pressed(key_state_from_action(ACTION_INTERACT, game)) ||
                     os_key_just_pressed(game.window->input.keys[OS_KEY_ESCAPE])))
                {
                  interactee.storage.is_inventory_open = false;
                }
              }
            }
            break;
            case ENTITY_ITEM:
            case ENTITY_PLAYER:
            case ENTITY_BLOCK:
            case ENTITY_CONVEYOR:
            case ENTITY_TYPE_COUNT:
              break;
          }
        }
      }
      else if (entity.type == ENTITY_STORAGE)
      {
        if (entity.storage.is_inventory_open)
        {
          // TODO: pull this out into some inventory_ui() function?
          // TODO: figure out a better way to get the position of this layout
          auto inventory = ui_layout_begin(
            "storage inventory",
            game.ui_system,
            game.window->input,
            game.assets,
            {100 - 64, 200},
            {1280, 720},
            CHAR_SIZE,
            game.font_texture
          );
          {
            ui_element_begin(inventory, UI_AUTO_ID);
            // TODO: the fixed height here is icky
            // TODO: add new scrolling behaviour in ui that would match this use case
            defer(ui_element_end(
              inventory,
              {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
               .sizing = {ui_sizing_fit(), ui_sizing_fixed(400)},
               .bg_color = {0.7f, 0.7f, 0.7f, 1},
               .scroll_value = &entity.storage.scroll_value}
            ));

            // TODO: get rid of the double padding in between rows
            for (usize row = 1;
                 row < (EntityStorage::INVENTORY_SIZE / EntityPlayer::HOTBAR_SLOT_COUNT);
                 ++row)
            {
              ui_element_begin(inventory, UI_AUTO_ID);
              defer(ui_element_end(
                inventory,
                {.padding = ui_padding_all(4), .child_gap = 4, .bg_color = {0.7f, 0.7f, 0.7f, 1}}
              ));

              for (usize slot_idx = 0; slot_idx < EntityPlayer::HOTBAR_SLOT_COUNT; ++slot_idx)
              {
                auto& slot =
                  entity.storage.inventory[(row * EntityPlayer::HOTBAR_SLOT_COUNT) + slot_idx];
                if (inventory_slot_ui(inventory, slot, false, game))
                {
                  // TODO: fuckin hate this, should be fixed after entity refactor tho
                  ASSERT(player, "player has to be set");
                  handle_hand_slot_interaction(player->player.hand, slot);
                }
              }
            }
          }
          ui_layout_end(inventory);
        }
      }
      else if (entity.type == ENTITY_ITEM)
      {
        ASSERT(entity.item.slot.count > 0, "Item entities cannot have an empty slot");
        // NOTE: item rotation
        {
          entity.item.rotation += EntityItem::ROTATION_SPEED * dt;
          entity.item.rotation = wrap_to_neg_pi_to_pi(entity.item.rotation);
        }
      }
    }
  }

  // NOTE: ui
  bool mouse_over_hotbar = false;
  {
    // TODO: figure out a better way to get the position of this layout
    auto hotbar = ui_layout_begin(
      "block selection menu",
      game.ui_system,
      game.window->input,
      game.assets,
      {100 - 64, game.window->dimensions.y - 100},
      {1280, 720},
      CHAR_SIZE,
      game.font_texture
    );
    {
      ui_element_begin(hotbar, UI_AUTO_ID);
      defer(ui_element_end(
        hotbar,
        {.padding = ui_padding_all(4), .child_gap = 4, .bg_color = {0.7f, 0.7f, 0.7f, 1}}
      ));

      for (u8 hotbar_slot_idx = 0; hotbar_slot_idx < EntityPlayer::HOTBAR_SLOT_COUNT;
           ++hotbar_slot_idx)
      {
        auto& slot = player->player.inventory[hotbar_slot_idx];
        if (inventory_slot_ui(
              hotbar,
              slot,
              hotbar_slot_idx == player->player.selected_hotbar_slot,
              game
            ))
        {
          handle_hand_slot_interaction(player->player.hand, slot);
        }
      }
    }
    ui_layout_end(hotbar);
    // TODO: change the access of the first element to a helper function instead?
    mouse_over_hotbar = ui_intersects(
      game.window->input.mouse_pos,
      {hotbar.pos.x, hotbar.pos.y},
      hotbar.elements[1].dimensions
    );
  }
  bool mouse_over_inventory = false;
  if (player->player.is_inventory_open)
  {
    // TODO: pull this out into some inventory_ui() function?
    // TODO: figure out a better way to get the position of this layout
    auto inventory = ui_layout_begin(
      "player inventory",
      game.ui_system,
      game.window->input,
      game.assets,
      {100 - 64, game.window->dimensions.y - 400},
      {1280, 720},
      CHAR_SIZE,
      game.font_texture
    );
    {
      ui_element_begin(inventory, UI_AUTO_ID);
      defer(ui_element_end(inventory, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL}));

      // TODO: get rid of the double padding in between rows
      for (usize row = 1; row < (EntityPlayer::INVENTORY_SIZE / EntityPlayer::HOTBAR_SLOT_COUNT);
           ++row)
      {
        ui_element_begin(inventory, UI_AUTO_ID);
        defer(ui_element_end(
          inventory,
          {.padding = ui_padding_all(4), .child_gap = 4, .bg_color = {0.7f, 0.7f, 0.7f, 1}}
        ));

        for (usize slot_idx = 0; slot_idx < EntityPlayer::HOTBAR_SLOT_COUNT; ++slot_idx)
        {
          auto& slot = player->player.inventory[(row * EntityPlayer::HOTBAR_SLOT_COUNT) + slot_idx];
          if (inventory_slot_ui(inventory, slot, false, game))
          {
            handle_hand_slot_interaction(player->player.hand, slot);
          }
        }
      }
    }
    ui_layout_end(inventory);
    // TODO: change the access of the first element to a helper function instead?
    mouse_over_inventory = ui_intersects(
      game.window->input.mouse_pos,
      {inventory.pos.x, inventory.pos.y},
      inventory.elements[1].dimensions
    );
  }
  game.mouse_over_player_inventory = mouse_over_hotbar || mouse_over_inventory;
  if (game.debug.menu.shown)
  {
    {
      auto debug_menu = ui_layout_begin(
        "debug menu",
        game.ui_system,
        game.window->input,
        game.assets,
        game.debug.menu.pos,
        {1280, 720},
        CHAR_SIZE,
        game.font_texture
      );
      {
        ui_element_begin(debug_menu, UI_AUTO_ID);
        defer(ui_element_end(debug_menu, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL}));

        {
          bool titlebar_clicked = false;
          ui_element_begin(debug_menu, UI_AUTO_ID, {.clicked = &titlebar_clicked});
          defer(ui_element_end(
            debug_menu,
            {.sizing = {ui_sizing_fill(), ui_sizing_fit()},
             .padding = ui_padding_all(4),
             .child_gap = 4,
             .child_alignment = {.y = UI_CHILD_ALIGNMENT_CENTER},
             .bg_color = {0, 0, 0, 0.5f}}
          ));

          {
            bool collapser_clicked = false;
            ui_element_begin(debug_menu, UI_AUTO_ID, {.clicked = &collapser_clicked});
            defer(ui_element_end(
              debug_menu,
              {.sizing = {ui_sizing_fixed(16), ui_sizing_fixed(16)},
               .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
               .bg_color = {0.5f, 0.5f, 0.5f, 1}}
            ));
            if (game.debug.menu.open)
            {
              ui_text(debug_menu, "v", 1.0f);
            }
            else
            {
              ui_text(debug_menu, "-", 1.0f);
            }
            if (collapser_clicked)
            {
              game.debug.menu.open = !game.debug.menu.open;
            }
          }

          ui_text(debug_menu, "debug window (F1)", 1.0f);

          if (titlebar_clicked)
          {
            game.debug.menu.drag = true;
            game.debug.menu.drag_offset =
              game.window->input.mouse_pos - vec2{debug_menu.pos.x, debug_menu.pos.y};
          }
          if (game.debug.menu.drag)
          {
            if (game.window->input.lmb.down)
            {
              vec2 new_pos = game.window->input.mouse_pos - game.debug.menu.drag_offset;
              game.debug.menu.pos.x = new_pos.x;
              game.debug.menu.pos.y = new_pos.y;
            }
            else
            {
              game.debug.menu.drag = false;
            }
          }
        }

        if (game.debug.menu.open)
        {
          ui_element_begin(debug_menu, UI_AUTO_ID);
          defer(ui_element_end(
            debug_menu,
            {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
             .padding = ui_padding_all(4),
             .child_gap = 4,
             .bg_color = {0.2f, 0.2f, 0.2f, 0.5f}}
          ));

          auto checkbox = [](UI_Layout& layout, UI_Id id, bool& value, std::string_view text)
          {
            bool checkbox_test = false;
            ui_element_begin(layout, UI_AUTO_ID);
            defer(ui_element_end(
              layout,
              {.layout_direction = UI_LAYOUT_DIRECTION_HORIZONTAL, .child_gap = 4}
            ));

            ui_element_begin(layout, id, {.clicked = &checkbox_test});
            ui_element_end(
              layout,
              {.sizing = {ui_sizing_fixed(16), ui_sizing_fixed(16)},
               .bg_color = value ? vec4{1, 1, 1, 1} : vec4{0.4f, 0.4f, 0.4f, 1},
               .corner_radius = 0.5f}
            );
            if (checkbox_test)
            {
              value = !value;
            }
            ui_text(layout, text, 1.0f);
          };
          checkbox(
            debug_menu,
            UI_AUTO_ID,
            game.debug.display_bounding_boxes,
            "display bounding boxes"
          );
          checkbox(debug_menu, UI_AUTO_ID, game.debug.noclip, "noclip (F2)");
        }
      }
      ui_layout_end(debug_menu);
    }
    {
      auto error_list = ui_layout_begin(
        "error list",
        game.ui_system,
        game.window->input,
        game.assets,
        game.debug.error_list.pos,
        {1280, 720},
        CHAR_SIZE,
        game.font_texture
      );
      {
        ui_element_begin(error_list, UI_AUTO_ID);
        defer(ui_element_end(error_list, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL}));

        {
          bool titlebar_clicked = false;
          ui_element_begin(error_list, UI_AUTO_ID, {.clicked = &titlebar_clicked});
          defer(ui_element_end(
            error_list,
            {.sizing = {ui_sizing_fill(), ui_sizing_fit()},
             .padding = ui_padding_all(4),
             .child_gap = 4,
             .child_alignment = {.y = UI_CHILD_ALIGNMENT_CENTER},
             .bg_color = {0.8f, 0, 0, 0.5f}}
          ));

          {
            bool collapser_clicked = false;
            ui_element_begin(error_list, UI_AUTO_ID, {.clicked = &collapser_clicked});
            defer(ui_element_end(
              error_list,
              {.sizing = {ui_sizing_fixed(16), ui_sizing_fixed(16)},
               .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
               .bg_color = {0.8f, 0.5f, 0.5f, 1}}
            ));
            if (game.debug.error_list.open)
            {
              ui_text(error_list, "v", 1.0f);
            }
            else
            {
              ui_text(error_list, "-", 1.0f);
            }
            if (collapser_clicked)
            {
              game.debug.error_list.open = !game.debug.error_list.open;
            }
          }

          ui_text(error_list, std::format("{} errors (F1)", g_error_list.size()), 1.0f);

          if (titlebar_clicked)
          {
            game.debug.error_list.drag = true;
            game.debug.error_list.drag_offset =
              game.window->input.mouse_pos - vec2{error_list.pos.x, error_list.pos.y};
          }
          if (game.debug.error_list.drag)
          {
            if (game.window->input.lmb.down)
            {
              vec2 new_pos = game.window->input.mouse_pos - game.debug.error_list.drag_offset;
              game.debug.error_list.pos.x = new_pos.x;
              game.debug.error_list.pos.y = new_pos.y;
            }
            else
            {
              game.debug.error_list.drag = false;
            }
          }
        }

        if (game.debug.error_list.open && !g_error_list.empty())
        {
          ui_element_begin(error_list, UI_AUTO_ID);
          defer(ui_element_end(
            error_list,
            {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
             .padding = ui_padding_all(4),
             .child_gap = 4,
             .bg_color = {0.2f, 0.2f, 0.2f, 0.5f}}
          ));

          for (usize i = 0; i < g_error_list.size(); ++i)
          {
            auto& err = g_error_list[i];
            ui_text(
              error_list,
              std::format("{}. ({}:{}) {}", i + 1, err.function, err.line, err.message),
              1.0f
            );
          }
        }
      }
      ui_layout_end(error_list);
    }
  }
}

void game_update_frame(GameData& game, f32 t)
{
  UNUSED(t);
  if (game.debug.noclip)
  {
    vec2 window_center =
      (vec2{(f32) game.window->dimensions.x, (f32) game.window->dimensions.y} / 2.0f);
    vec2 delta = game.window->input.mouse_pos - window_center;
    vec2 offset = delta * CAMERA_SENSITIVITY;
    camera_look_around(game.debug_camera, offset);
  }
}

void game_render(GameData& game, f32 t)
{
  // NOTE: shadow map pass
  Camera shadow_map_camera{};
  {
    vec3 pos = {};
    for (usize i = 0; i < game.scene.entities.size(); ++i)
    {
      auto& entity = game.scene.entities[i];
      if (entity.type == ENTITY_LIGHT_BULB)
      {
        pos = entity.pos;
        pos.y += EntityLightBulb::LIGHT_HEIGHT_OFFSET;
        break;
      }
    }

    shadow_map_camera = {
      .type = CAMERA_TYPE_PERSPECTIVE,
      .pos = pos,
      .prev_pos = pos,
      .yaw = std::numbers::pi_v<f32>,
      .fov_type = FOV_TYPE_VERTICAL,
      .fov = 0.5f * std::numbers::pi_v<f32>,
      .near_plane = 0.1f,
      .far_plane = 25.0f,
      .viewport = SHADOW_MAP_DIMENSIONS,
    };
    camera_update_vectors(shadow_map_camera);

    auto& c = shadow_map_camera;
    mat4 light_proj_mat = camera_projection(c);
    std::array<mat4, 6> transforms = {
      light_proj_mat * look_at(c.pos, c.pos + vec3{1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * look_at(c.pos, c.pos + vec3{-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * look_at(c.pos, c.pos + vec3{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
      light_proj_mat * look_at(c.pos, c.pos + vec3{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}),
      light_proj_mat * look_at(c.pos, c.pos + vec3{0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * look_at(c.pos, c.pos + vec3{0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}),
    };

    Render_Pass pass = {
      .t = t,
      .camera = &shadow_map_camera,
    };
    render_pass_render_to(pass, game.shadow_map);
    render_pass_override_shader(pass, game.shadow_depth_shader);
    render_pass_on_shader_bind(
      pass,
      [&transforms](Shader& shader)
      {
        shader_set(shader, "shadow_matrices[0]", transforms[0]);
        shader_set(shader, "shadow_matrices[1]", transforms[1]);
        shader_set(shader, "shadow_matrices[2]", transforms[2]);
        shader_set(shader, "shadow_matrices[3]", transforms[3]);
        shader_set(shader, "shadow_matrices[4]", transforms[4]);
        shader_set(shader, "shadow_matrices[5]", transforms[5]);
      }
    );

    for (usize i = 0; i < game.scene.entities.size(); ++i)
    {
      auto& entity = game.scene.entities[i];
      if (f32_equal(entity.pos.y, 0) && entity.type != ENTITY_LIGHT_BULB)
      {
        render_mesh(
          pass.cmds_3d,
          entity.mesh,
          entity_render_pos(entity, t),
          entity_render_rotation(entity, t),
          {1, 1, 1},
          entity.scale,
          game.assets
        );
      }
    }

    render_pass_finish(pass, game.assets);
  }

  // NOTE: main draw pass
  {
    Render_Pass pass = {
      .t = t,
      .camera = game.used_camera,
      .ambient_color = game.scene.ambient_color,
    };
    render_pass_on_shader_bind(
      pass,
      [&](Shader& shader)
      {
        shader_set(shader, "shadow_map", game.shadow_map, game.assets);
        shader_set(shader, "shadow_map_camera_far_plane", shadow_map_camera.far_plane);
      }
    );

    for (usize i = 0; i < game.scene.entities.size(); ++i)
    {
      const auto& entity = game.scene.entities[i];
      switch (entity.type)
      {
        case ENTITY_PLAYER:
        {
          if (entity.player.hand.count > 0)
          {
            pass.cmds_2d.push_back(render_texture(
              game.item_icons[entity.player.hand.type],
              {game.window->input.mouse_pos.x - 24, game.window->input.mouse_pos.y - 24, 0.9f},
              {48, 48}
            ));
          }

          // TODO: should be if
          // - the selected hotbar slot count != 0 OR currently hovering over some removable entity
          // - AND not hovering over any interactable
          // - AND the mouse is not over the players inventory
          if (game.mouse_in_player_interaction_radius &&
              player_selected_hotbar_slot(entity).count != 0 && !game.mouse_over_player_inventory &&
              entity.player.hand.count == 0)
          {
            // TODO: dont use EntityLightBulb::OFF_HOVER_COLOR, it should be more generic
            if (ENTITY_ROTATABLE[player_selected_hotbar_slot_entity_type(entity)])
            {
              render_line_arrow(
                pass.cmds_3d,
                game.mouse_tile_pos,
                0.6f,
                0.3f,
                (f32) game.scene.place_rotation * (0.5f * std::numbers::pi_v<f32>),
                EntityLightBulb::OFF_HOVER_COLOR,
                game.assets
              );
            }
            pass.cmds_3d.push_back(render_cube_wires(
              game.mouse_tile_pos,
              {1, 1, 1},
              EntityLightBulb::OFF_HOVER_COLOR,
              game.assets
            ));
          }
          if (game.debug.display_bounding_boxes)
          {
            pass.cmds_3d.push_back(render_line(
              entity_render_pos(entity, t),
              0.6f,
              entity_render_rotation(entity, t),
              {1, 0, 0},
              game.assets
            ));
            pass.cmds_3d.push_back(render_ring(
              entity_render_pos(entity, t),
              EntityPlayer::INTERACTION_RADIUS,
              {1, 1, 0},
              game.assets
            ));
          }
        }
        break;

        case ENTITY_LIGHT_BULB:
        {
          if (entity.light_bulb.hovered)
          {
            pass.cmds_3d.push_back(render_cube_wires(
              entity_render_pos(entity, t),
              {ENTITY_BOUNDING_BOX[entity.type].x, 1, ENTITY_BOUNDING_BOX[entity.type].y},
              entity.light_bulb.on ? EntityLightBulb::ON_HOVER_COLOR
                                   : EntityLightBulb::OFF_HOVER_COLOR,
              game.assets
            ));
          }
          if (entity.light_bulb.on)
          {
            vec3 pos = entity_render_pos(entity, t);
            pos.y += EntityLightBulb::LIGHT_HEIGHT_OFFSET;
            render_pass_set_light(pass, pos, EntityLightBulb::LIGHT_COLOR);
          }
        }
        break;

        case ENTITY_STORAGE:
        {
          if (entity.storage.hovered && !entity.storage.is_inventory_open)
          {
            pass.cmds_3d.push_back(render_cube_wires(
              entity_render_pos(entity, t),
              {ENTITY_BOUNDING_BOX[entity.type].x, 1, ENTITY_BOUNDING_BOX[entity.type].y},
              EntityLightBulb::OFF_HOVER_COLOR,
              game.assets
            ));
          }
          else if (entity.storage.is_inventory_open)
          {
            pass.cmds_3d.push_back(render_cube_wires(
              entity_render_pos(entity, t),
              {ENTITY_BOUNDING_BOX[entity.type].x, 1, ENTITY_BOUNDING_BOX[entity.type].y},
              EntityLightBulb::ON_HOVER_COLOR,
              game.assets
            ));
          }
        }
        break;

        // TODO: items should be rendered with the default shader not the lighting one
        case ENTITY_ITEM:
        case ENTITY_BLOCK:
        case ENTITY_CONVEYOR:
        case ENTITY_TYPE_COUNT:
          break;
      }

      render_mesh(
        pass.cmds_3d,
        entity.mesh,
        entity_render_pos(entity, t),
        entity_render_rotation(entity, t),
        entity.tint,
        entity.scale,
        game.assets
      );

      if (game.debug.display_bounding_boxes && f32_equal(entity.pos.y, 0))
      {
        pass.cmds_3d.push_back(render_cube_wires(
          entity_render_pos(entity, t),
          {ENTITY_BOUNDING_BOX[entity.type].x, 1, ENTITY_BOUNDING_BOX[entity.type].y},
          {0, 1, 0},
          game.assets
        ));
      }
    }

    pass.cmds_2d.insert(
      pass.cmds_2d.end(),
      game.ui_system.render_cmds.begin(),
      game.ui_system.render_cmds.end()
    );

    render_pass_finish(pass, game.assets);
  }
}
