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
    .rendered_pos = {0, 12, 8},
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
  render_init();
  {
    auto scene = scene_from_file("data/main.gscn");
    ASSERT(scene, "Failed to load scene {}", scene.error());
    game.scene = *scene;
  }
  game.shadow_map = asset_set(g_assets, texture_init(TEXTURE_CUBEMAP, SHADOW_MAP_DIMENSIONS));
  {
    auto shader = shader_from_file(
      "shaders/shadow_depth.vert",
      "shaders/shadow_depth.frag",
      "shaders/shadow_depth.geom"
    );
    if (shader)
    {
      game.shadow_depth_shader = asset_set(g_assets, *shader);
    }
    else
    {
      REPORT_ERROR(shader.error());
    }
  }
  render_create_framebuffer(game.shadow_map);
  // TODO: load this with FilterOption::NEAREST
  game.font_texture = load_texture(g_assets, "assets/font.png");
  // m_sound_system.play_looped(SoundHandle::TEST_MUSIC, 0.1f);
  return game;
}

void game_deinit(GameData& game)
{
  sound_system_deinit(game.sound_system);
}

void game_update_tick(GameData& game, f32 dt)
{
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

    for (usize i = 0; i < game.scene.entities.size(); ++i)
    {
      auto& entity = game.scene.entities[i];
      entity.prev_pos = entity.pos;
      entity.prev_rotation = entity.rotation;

      if (entity.flags & ENTITY_CONTROLLED_BY_PLAYER)
      {
        // NOTE: rotation
        {
          if (acceleration != vec3{0.0f, 0.0f, 0.0f})
          {
            auto rot = std::atan2(-acceleration.x, acceleration.z);
            entity.target_rotation = rot;
          }
          f32 direction = wrap_to_neg_pi_to_pi(entity.target_rotation - entity.rotation);
          entity.rotation += direction * PLAYER_ROTATE_SPEED * dt;
          entity.rotation = wrap_to_neg_pi_to_pi(entity.rotation);
        }

        // NOTE: movement and collisions
        // TODO: this is still a little wrong in some cases,
        // maybe use a better algorithm overall to improve this
        {
          acceleration *= PLAYER_MOVEMENT_SPEED;

          static const f32 friction_coefficient = 0.35f;
          static const f32 normal_force = PLAYER_MASS * G_F32;
          static const f32 friction_magnitude = friction_coefficient * normal_force;

          vec3 friction_dir = -normalize(entity.velocity);
          vec3 friction_force = friction_dir * friction_magnitude;

          vec3 drag = -3.0f * entity.velocity;
          vec3 friction = (friction_force / PLAYER_MASS) + drag;

          acceleration += friction;
          auto new_pos = 0.5f * acceleration * (dt * dt) + entity.velocity * dt + entity.pos;
          entity.velocity = acceleration * dt + entity.velocity;

          vec3 collision_normal = {};
          bool collided = false;
          for (usize collidable_idx = 0; collidable_idx < game.scene.entities.size();
               ++collidable_idx)
          {
            auto& c = game.scene.entities[collidable_idx];
            if (&c == &entity || !(c.flags & ENTITY_COLLIDABLE) || !f32_equal(c.pos.y, 0))
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
            p.pos = new_pos;
            p.bounding_box = entity.bounding_box;

            if (!entities_collide(p, c))
            {
              continue;
            }

            auto collidable_front = c.pos.z + (0.5f * c.bounding_box.y);
            auto collidable_back = c.pos.z - (0.5f * c.bounding_box.y);
            auto collidable_left = c.pos.x - (0.5f * c.bounding_box.x);
            auto collidable_right = c.pos.x + (0.5f * c.bounding_box.x);

            auto entity_front = p.pos.z + (0.5f * p.bounding_box.y);
            auto entity_back = p.pos.z - (0.5f * p.bounding_box.y);
            auto entity_left = p.pos.x - (0.5f * p.bounding_box.x);
            auto entity_right = p.pos.x + (0.5f * p.bounding_box.x);

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
            entity.velocity -= dot(entity.velocity, collision_normal) * collision_normal;
            sound_play_once(game.sound_system, SOUND_HANDLE_SINE, 0.1f);
          }
        }

        // NOTE: interactions
        if (os_key_just_pressed(key_state_from_action(ACTION_INTERACT, game)))
        {
          for (usize interactable_idx = 0; interactable_idx < game.scene.entities.size();
               ++interactable_idx)
          {
            auto& interactable = game.scene.entities[interactable_idx];
            if (!(interactable.flags & ENTITY_TOGGLEABLE))
            {
              continue;
            }
            auto vec = interactable.pos - entity.pos;
            f32 dist = length2(vec);
            f32 orientation = std::atan2(-vec.x, vec.z);
            orientation = wrap_to_neg_pi_to_pi(orientation);
            if (dist < (interactable.interactable_radius * interactable.interactable_radius) &&
                std::abs(entity.rotation - orientation) < 1.0f)
            {
              // TODO: this is light bulb specific behaviour, how do i work with other
              // interactables?
              interactable.flags ^= ENTITY_EMITS_LIGHT;
              interactable.tint =
                interactable.flags & ENTITY_EMITS_LIGHT ? LIGHT_BULB_ON_TINT : LIGHT_BULB_OFF_TINT;
              sound_play_once(game.sound_system, SOUND_HANDLE_SHOTGUN, 0.1f);
              sound_stop_looped(game.sound_system, SOUND_HANDLE_TEST_MUSIC);
            }
          }
        }
      }
    }
  }

  // NOTE: ui
  ui_system_update(game.ui_system);
  if (game.debug.menu.shown)
  {
    {
      auto debug_menu = ui_layout_begin(
        "debug menu",
        game.ui_system,
        game.window->input,
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

void game_update_frame(GameData& game, f32 alpha)
{
  if (game.debug.noclip)
  {
    vec2 offset = game.window->input.mouse_delta * CAMERA_SENSITIVITY;
    camera_look_around(game.debug_camera, offset);
    camera_update(game.debug_camera, alpha);
  }
  else
  {
    for (usize i = 0; i < game.scene.entities.size(); ++i)
    {
      auto& entity = game.scene.entities[i];
      entity.rendered_pos = entity.pos * alpha + entity.prev_pos * (1.0f - alpha);
      vec2 prev_rot_vec = {-std::sin(entity.prev_rotation), std::cos(entity.prev_rotation)};
      vec2 rot_vec = {-std::sin(entity.rotation), std::cos(entity.rotation)};
      vec2 rendered_rot_vec = rot_vec * alpha + prev_rot_vec * (1.0f - alpha);
      entity.rendered_rotation = std::atan2(-rendered_rot_vec.x, rendered_rot_vec.y);
    }
  }
}

void game_render(GameData& game)
{
  // NOTE: shadow map pass
  Camera shadow_map_camera{};
  {
    vec3 pos = {};
    for (usize i = 0; i < game.scene.entities.size(); ++i)
    {
      auto& entity = game.scene.entities[i];
      if (entity.flags & ENTITY_EMITS_LIGHT)
      {
        pos = entity.pos;
        pos.y += entity.light_height_offset;
        break;
      }
    }

    shadow_map_camera = {
      .type = CAMERA_TYPE_PERSPECTIVE,
      .pos = pos,
      .prev_pos = pos,
      .rendered_pos = pos,
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
      if (entity.flags & ENTITY_CONTROLLED_BY_PLAYER && entity.flags & ENTITY_VISIBLE)
      {
        render_pass_append(
          pass,
          render_mesh(entity.mesh, entity.rendered_pos, entity.rendered_rotation)
        );
      }
    }

    render_pass_finish(pass);
  }

  // NOTE: main draw pass
  {
    Render_Pass pass = {
      .camera = game.used_camera,
      .ambient_color = game.scene.ambient_color,
    };
    render_pass_on_shader_bind(
      pass,
      [&](Shader& shader)
      {
        shader_set(shader, "shadow_map", game.shadow_map);
        shader_set(shader, "shadow_map_camera_far_plane", shadow_map_camera.far_plane);
      }
    );

    for (usize i = 0; i < game.scene.entities.size(); ++i)
    {
      const auto& entity = game.scene.entities[i];
      if (entity.flags & ENTITY_VISIBLE)
      {
        render_pass_append(
          pass,
          render_mesh(entity.mesh, entity.rendered_pos, entity.rendered_rotation, entity.tint)
        );
      }
      if (entity.flags & ENTITY_EMITS_LIGHT)
      {
        vec3 pos = entity.rendered_pos;
        pos.y += entity.light_height_offset;
        render_pass_set_light(pass, pos, entity.light_color);
      }

      if (game.debug.display_bounding_boxes)
      {
        if (entity.flags & ENTITY_CONTROLLED_BY_PLAYER)
        {
          render_pass_append(
            pass,
            render_line(entity.rendered_pos, 0.6f, entity.rendered_rotation, {1.0f, 0.0f, 0.0f})
          );
        }
        if (entity.flags & ENTITY_COLLIDABLE && f32_equal(entity.pos.y, 0.0f))
        {
          render_pass_append(
            pass,
            render_cube_wires(
              entity.rendered_pos,
              {entity.bounding_box.x, 1.0f, entity.bounding_box.y},
              {0.0f, 1.0f, 0.0f}
            )
          );
        }
        if (entity.flags & ENTITY_TOGGLEABLE)
        {
          render_pass_append(
            pass,
            render_ring(entity.rendered_pos, entity.interactable_radius, {1.0f, 1.0f, 0.0f})
          );
        }
      }
    }

    render_pass_append(pass, game.ui_system.render_cmds);

    render_pass_finish(pass);
  }
}
