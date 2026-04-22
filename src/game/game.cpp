#include "game.h"

#include <cmath>
#include <fstream>
#include <string>

#include "base/base.h"
#include "camera.h"
#include "os/os.h"
#include "renderer.h"
#include "entity.h"
#include "parser.h"
#include "sdl3/src/video/khronos/vulkan/vulkan_core.h"
#include "ui.h"

std::expected<std::string_view, std::string_view> action_to_string(Action action)
{
  switch (action)
  {
    case Action::MOVE_FRONT:
      return {"move_front"};
    case Action::MOVE_BACK:
      return {"move_back"};
    case Action::MOVE_LEFT:
      return {"move_left"};
    case Action::MOVE_RIGHT:
      return {"move_right"};
    case Action::INTERACT:
      return {"interact"};
    case Action::CAMERA_MOVE_UP:
      return {"camera_move_up"};
    case Action::CAMERA_MOVE_DOWN:
      return {"camera_move_down"};
    case Action::TOGGLE_DEBUG_MENU:
      return {"toggle_debug_menu"};
    case Action::TOGGLE_CAMERA_MODE:
      return {"toggle_camera_mode"};
    case Action::COUNT:
    default:
      return std::unexpected{"Invalid action."};
  }
}

Keymap load_gkey(const std::filesystem::path& path)
{
  Keymap keymap{};
  std::ifstream file{path};
  ASSERT(!file.fail(), "File reading error.");
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    parser::Pos pos{.line = line};

    auto action = parser::word(pos);
    parser::expect_and_skip(pos, ':');
    auto key_str = parser::word(pos);
    auto key = os::string_to_key(key_str);
    ASSERT(key, "Invalid key string. ({})", key.error());
    for (usize i = 0; i < keymap.size(); ++i)
    {
      auto action_str = action_to_string((Action) i);
      ASSERT(action_str, "Invalid action. ({})", action_str.error());
      if (action == action_str)
      {
        keymap[(Action) i] = *key;
      }
    }
  }
  return keymap;
}

Game::Game(os::Window& window, os::Audio& audio)
  : m_window{window}, m_sound_system{audio}, m_keymap{load_gkey("data/keymap.gkey")},
    m_gameplay_camera{CameraDescription{
      .type = CameraType::PERSPECTIVE,
      .pos = {0, 12, 8},
      .yaw = -90,
      .pitch = -55,
      .using_vertical_fov = true,
      .fov = 0.25f * std::numbers::pi_v<f32>,
      .near_plane = 0.1f,
      .far_plane = 1000.0f,
      .viewport = m_window.dimensions(),
    }},
    m_debug_camera{m_gameplay_camera}, m_main_camera{&m_gameplay_camera}
{
  render::init();
  scene = load_scene("data/main.gscn");
  // m_sound_system.play_looped(SoundHandle::TEST_MUSIC, 0.1f);
  shadow_map = AssetManager::instance().set(Texture{TextureType::CUBEMAP, SHADOW_MAP_DIMENSIONS});
  shadow_depth_shader = AssetManager::instance().set(
    Shader{"shaders/shadow_depth.vert", "shaders/shadow_depth.frag", "shaders/shadow_depth.geom"}
  );
  render::create_framebuffer(shadow_map);
  // TODO: load this with FilterOption::NEAREST
  font_texture = AssetManager::instance().load_texture("assets/font.png");
}

void Game::update_tick(f32 dt)
{
  m_gameplay_camera.update_viewport(m_window.dimensions());
  m_debug_camera.update_viewport(m_window.dimensions());

  if (action_key(Action::TOGGLE_DEBUG_MENU).just_pressed())
  {
    debug_menu_shown = !debug_menu_shown;
    debug_menu_drag = false;
    ui_render_cmds.clear();
  }
  if (action_key(Action::TOGGLE_CAMERA_MODE).just_pressed())
  {
    m_camera_mode = !m_camera_mode;
  }

  vec3 acceleration = {};
  if (action_key(Action::MOVE_FRONT).down)
  {
    acceleration.z += -1.0f;
  }
  if (action_key(Action::MOVE_BACK).down)
  {
    acceleration.z += 1.0f;
  }
  if (action_key(Action::MOVE_LEFT).down)
  {
    acceleration.x += -1.0f;
  }
  if (action_key(Action::MOVE_RIGHT).down)
  {
    acceleration.x += 1.0f;
  }
  acceleration = acceleration.normalize();

  if (m_camera_mode)
  {
    m_main_camera = &m_debug_camera;
    m_window.hide_mouse_pointer();

    if (action_key(Action::CAMERA_MOVE_UP).down)
    {
      acceleration.y += 1.0f;
    }
    if (action_key(Action::CAMERA_MOVE_DOWN).down)
    {
      acceleration.y += -1.0f;
    }

    auto forward = cross(Camera::WORLD_UP, m_debug_camera.right());
    m_debug_camera.move(acceleration, forward, dt);
  }
  else
  {
    m_main_camera = &m_gameplay_camera;
    m_window.show_mouse_pointer();

    for (usize i = 0; i < scene.entities.size(); ++i)
    {
      auto& entity = scene.entities[i];
      entity.prev_pos = entity.pos;
      entity.prev_rotation = entity.rotation;

      if (entity.controlled_by_player())
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
          static const f32 normal_force = PLAYER_MASS * constants<f32>::G;
          static const f32 friction_magnitude = friction_coefficient * normal_force;

          vec3 friction_dir = -entity.velocity.normalize();
          vec3 friction_force = friction_dir * friction_magnitude;

          vec3 drag = -3.0f * entity.velocity;
          vec3 friction = (friction_force / PLAYER_MASS) + drag;

          acceleration += friction;
          auto new_pos = 0.5f * acceleration * (dt * dt) + entity.velocity * dt + entity.pos;
          entity.velocity = acceleration * dt + entity.velocity;

          vec3 collision_normal = {};
          bool collided = false;
          for (usize collidable_idx = 0; collidable_idx < scene.entities.size(); ++collidable_idx)
          {
            auto& c = scene.entities[collidable_idx];
            if (&c == &entity || !c.collidable() || c.pos.y != 0.0f)
            {
              continue;
            }

            vec3 rounded_pos = {std::round(new_pos.x), 0.0f, std::round(new_pos.z)};
            if (
              (c.pos.x > rounded_pos.x + 1.0f || c.pos.x < rounded_pos.x - 1.0f) ||
              (c.pos.z > rounded_pos.z + 1.0f || c.pos.z < rounded_pos.z - 1.0f)
            )
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
            m_sound_system.play_once(SoundHandle::SINE, 0.1f);
          }
        }

        // NOTE: interactions
        if (action_key(Action::INTERACT).just_pressed())
        {
          for (usize interactable_idx = 0; interactable_idx < scene.entities.size();
               ++interactable_idx)
          {
            auto& interactable = scene.entities[interactable_idx];
            if (!interactable.interactable())
            {
              continue;
            }
            auto vec = interactable.pos - entity.pos;
            f32 dist = vec.length2();
            f32 orientation = std::atan2(-vec.x, vec.z);
            orientation = wrap_to_neg_pi_to_pi(orientation);
            if (
              dist < (interactable.interactable_radius * interactable.interactable_radius) &&
              std::abs(entity.rotation - orientation) < 1.0f
            )
            {
              // TODO: this is light bulb specific behaviour, how do i work with other
              // interactables?
              interactable.flags ^= Entity::EMITS_LIGHT;
              interactable.tint =
                interactable.emits_light() ? LIGHT_BULB_ON_TINT : LIGHT_BULB_OFF_TINT;
              m_sound_system.play_once(SoundHandle::SHOTGUN, 0.1f);
              m_sound_system.stop_looped(SoundHandle::TEST_MUSIC);
            }
          }
        }
      }
    }
  }

  // NOTE: ui
  auto layout = ui_begin_layout(m_window.input(), layout_pos, {1280, 720}, {9, 16}, font_texture);
  UI_ELEM(
    layout,
    {.sizing = {UI_SizingAxis::fill(), UI_SizingAxis::fill()}, .bg_color = {1, 1, 1, 1}}
  )
  {
    UI_ELEM(
      layout,
      {.layout_direction = UI_LayoutDirection::VERTICAL,
       .sizing = {UI_SizingAxis::fixed(300), UI_SizingAxis::fill()},
       .bg_color = {0.85f, 0.8f, 0.8f, 1}}
    )
    {
      UI_ELEM(layout, {.sizing = {UI_SizingAxis::fill()}, .bg_color = {1, 0, 0, 1}})
      {
        UI_ELEM(
          layout,
          {.sizing = {UI_SizingAxis::fixed(60), UI_SizingAxis::fixed(60)}, .bg_color = {0, 1, 0, 1}}
        )
        {
        }
        ui_text(layout, "Test of UI library", 1.5f);
      }
      for (i32 i = 0; i < 5; ++i)
      {
        UI_ELEM(
          layout,
          {.sizing = {UI_SizingAxis::fill(), UI_SizingAxis::fixed(50)}, .bg_color = {0, 0, 1, 1}}
        )
        {
        }
      }
    }
    UI_ELEM(
      layout,
      {.sizing = {UI_SizingAxis::fill(), UI_SizingAxis::fill()}, .bg_color = {0.85f, 0.8f, 0.8f, 1}}
    )
    {
    }
  }
  ui_render_cmds = ui_end_layout(layout);
}

void Game::update_frame(f32 alpha)
{
  if (m_camera_mode)
  {
    vec2 offset = m_window.input().mouse_delta * Camera::SENSITIVITY;
    m_debug_camera.look_around(offset);

    m_debug_camera.update(alpha);
  }
  else
  {
    for (usize i = 0; i < scene.entities.size(); ++i)
    {
      auto& entity = scene.entities[i];
      entity.rendered_pos = entity.pos * alpha + entity.prev_pos * (1.0f - alpha);
      vec2 prev_rot_vec = {-std::sin(entity.prev_rotation), std::cos(entity.prev_rotation)};
      vec2 rot_vec = {-std::sin(entity.rotation), std::cos(entity.rotation)};
      vec2 rendered_rot_vec = rot_vec * alpha + prev_rot_vec * (1.0f - alpha);
      entity.rendered_rotation = std::atan2(-rendered_rot_vec.x, rendered_rot_vec.y);
    }
  }
}

void Game::render()
{
  // NOTE: shadow map pass
  Camera shadow_map_camera{};
  {
    vec3 pos = {};
    for (usize i = 0; i < scene.entities.size(); ++i)
    {
      auto& entity = scene.entities[i];
      if (entity.emits_light())
      {
        pos = entity.pos;
        pos.y += entity.light_height_offset;
        break;
      }
    }

    shadow_map_camera = Camera{
      {.pos = pos,
       .yaw = std::numbers::pi_v<f32>,
       .using_vertical_fov = false,
       .fov = std::numbers::pi_v<f32> * 0.5f,
       .near_plane = 0.1f,
       .far_plane = 25.0f,
       .viewport = SHADOW_MAP_DIMENSIONS}
    };
    auto& c = shadow_map_camera;
    mat4 light_proj_mat = c.projection();
    std::array<mat4, 6> transforms = {
      light_proj_mat * look_at(c.pos(), c.pos() + vec3{1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * look_at(c.pos(), c.pos() + vec3{-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * look_at(c.pos(), c.pos() + vec3{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
      light_proj_mat * look_at(c.pos(), c.pos() + vec3{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}),
      light_proj_mat * look_at(c.pos(), c.pos() + vec3{0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * look_at(c.pos(), c.pos() + vec3{0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}),
    };

    render::Pass pass{shadow_map_camera};
    pass.render_to(shadow_map);
    pass.override_shader(shadow_depth_shader);
    pass.on_shader_bind(
      [&transforms](Shader& shader)
      {
        shader.set("shadow_matrices[0]", transforms[0]);
        shader.set("shadow_matrices[1]", transforms[1]);
        shader.set("shadow_matrices[2]", transforms[2]);
        shader.set("shadow_matrices[3]", transforms[3]);
        shader.set("shadow_matrices[4]", transforms[4]);
        shader.set("shadow_matrices[5]", transforms[5]);
      }
    );

    for (usize i = 0; i < scene.entities.size(); ++i)
    {
      auto& entity = scene.entities[i];
      if (entity.controlled_by_player() && entity.renderable())
      {
        pass.append(render::mesh(entity.mesh, entity.rendered_pos, entity.rendered_rotation));
      }
    }

    pass.finish();
  }

  // NOTE: main draw pass
  {
    render::Pass pass{*m_main_camera, scene.ambient_color};
    pass.on_shader_bind(
      [&](Shader& shader)
      {
        shader.set("shadow_map", shadow_map);
        shader.set("shadow_map_camera_far_plane", shadow_map_camera.far_plane());
      }
    );

    for (usize i = 0; i < scene.entities.size(); ++i)
    {
      const auto& entity = scene.entities[i];
      if (entity.renderable())
      {
        pass.append(
          render::mesh(entity.mesh, entity.rendered_pos, entity.rendered_rotation, entity.tint)
        );
      }
      if (entity.emits_light())
      {
        vec3 pos = entity.rendered_pos;
        pos.y += entity.light_height_offset;
        pass.set_light(pos, entity.light_color);
      }

      if (m_display_bounding_boxes)
      {
        if (entity.controlled_by_player())
        {
          pass.append(
            render::line(entity.rendered_pos, 0.6f, entity.rendered_rotation, {1.0f, 0.0f, 0.0f})
          );
        }
        if (entity.collidable() && f32_equal(entity.pos.y, 0.0f))
        {
          pass.append(
            render::cube_wires(
              entity.rendered_pos,
              {entity.bounding_box.x, 1.0f, entity.bounding_box.y},
              {0.0f, 1.0f, 0.0f}
            )
          );
        }
        if (entity.interactable())
        {
          pass.append(
            render::ring(entity.rendered_pos, entity.interactable_radius, {1.0f, 1.0f, 0.0f})
          );
        }
      }
    }

    pass.append(ui_render_cmds);

    pass.finish();
  }
}
