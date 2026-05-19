#include "ui.h"

#include <cstdlib>
#include <optional>
#include <cmath>

#include "base/base.h"
#include "game/renderer.h"
#include "os/os.h"

void ui_system_update(UI_System& system)
{
  system.render_cmds.clear();
}

UI_Layout ui_layout_begin(
  UI_Id id,
  UI_System& system,
  const OS_Input& input,
  const vec3& pos,
  const vec2& max_dimensions,
  const vec2& char_size,
  TextureHandle font_texture
)
{
  UI_IdInternal id_internal = id ? std::hash<UI_Id>{}(id) : std::hash<usize>{}(system.next_auto_id);
  ++system.next_auto_id;
  UI_Layout layout = {
    .id = id_internal,
    .system = &system,
    .input = &input,
    .pos = pos,
    .max_dimensions = max_dimensions,
    .char_size = char_size,
    .font_texture = font_texture,
  };
  ui_element_begin(layout, nullptr, {});
  return layout;
}

enum UI_Axis
{
  UI_AXIS_X,
  UI_AXIS_Y,
};

constexpr static std::array<UI_LayoutDirection, 2> layout_direction_from_axis = []()
{
  std::array<UI_LayoutDirection, 2> t{};
  t[(usize) UI_AXIS_X] = UI_LAYOUT_DIRECTION_HORIZONTAL;
  t[(usize) UI_AXIS_Y] = UI_LAYOUT_DIRECTION_VERTICAL;
  return t;
}();

constexpr static std::array<UI_Axis, 2> axis_from_layout_direction = []()
{
  std::array<UI_Axis, 2> t{};
  t[(usize) UI_LAYOUT_DIRECTION_HORIZONTAL] = UI_AXIS_X;
  t[(usize) UI_LAYOUT_DIRECTION_VERTICAL] = UI_AXIS_Y;
  return t;
}();

static UI_SizingAxis& ui_sizing_from_axis(UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_AXIS_X)
  {
    return config.sizing.width;
  }
  else if (axis == UI_AXIS_Y)
  {
    return config.sizing.height;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32& ui_dimension_from_axis(UI_Element& elem, UI_Axis axis)
{
  if (axis == UI_AXIS_X)
  {
    return elem.dimensions.x;
  }
  else if (axis == UI_AXIS_Y)
  {
    return elem.dimensions.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 ui_total_padding_from_axis(UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_AXIS_X)
  {
    return config.padding.left + config.padding.right;
  }
  else if (axis == UI_AXIS_Y)
  {
    return config.padding.top + config.padding.down;
  }
  ASSERT(false, "Invalid axis provided");
}

static void ui_calculate_fit_fixed_sizing_axis(UI_Layout& layout, UI_ElementIdx idx, UI_Axis axis)
{
  auto& elem = layout.elements[idx];
  auto& config = elem.config.normal;
  auto& sizing = ui_sizing_from_axis(config, axis);
  switch (sizing.type)
  {
    case UI_SIZING_FIXED:
      ui_dimension_from_axis(elem, axis) = sizing.fixed_px;
      break;
    case UI_SIZING_FIT:
    case UI_SIZING_FILL:
      for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
           child_idx = layout.elements[child_idx].next_sibling)
      {
        auto& child = layout.elements[child_idx];
        if (config.layout_direction == layout_direction_from_axis[(usize) axis])
        {
          ui_dimension_from_axis(elem, axis) +=
            ui_dimension_from_axis(child, axis) + config.child_gap;
        }
        else
        {
          f32& dim = ui_dimension_from_axis(elem, axis);
          dim = std::max(dim, ui_dimension_from_axis(child, axis));
        }
      }
      break;
  }
  ui_dimension_from_axis(elem, axis) += ui_total_padding_from_axis(config, axis);
}

// NOTE: fill sizing elements are treated as fit sizing when
// the parent has a fit sizing on the same axis
// and the current layout direction corresponds to that axis (horizontal for x, vertical for y)
// NOT SURE if that is the 'correct' behaviour but it makes sense to me
static void ui_calculate_text_fit_fixed_sizing(UI_Layout& layout, UI_ElementIdx idx = 0)
{
  bool has_children = false;
  auto& elem = layout.elements[idx];
  for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx = layout.elements[child_idx].next_sibling)
  {
    has_children = true;
    ui_calculate_text_fit_fixed_sizing(layout, child_idx);
  }

  switch (elem.config.type)
  {
    case UI_ELEMENT_NORMAL:
    {
      ui_calculate_fit_fixed_sizing_axis(layout, idx, UI_AXIS_X);
      ui_calculate_fit_fixed_sizing_axis(layout, idx, UI_AXIS_Y);
      auto& config = elem.config.normal;
      if (has_children)
      {
        ui_dimension_from_axis(elem, axis_from_layout_direction[(usize) config.layout_direction]) -=
          config.child_gap;
      }
    }
    break;

    case UI_ELEMENT_TEXT:
    {
      auto& config = elem.config.text;
      auto& str = layout.strings[config.string_idx];
      elem.dimensions =
        vec2{layout.char_size.x * (f32) str.size(), layout.char_size.y} * config.size;
    }
    break;
  }
}

static void ui_calculate_fill_sizing_axis(UI_Layout& layout, UI_ElementIdx idx, UI_Axis axis)
{
  auto& elem = layout.elements[idx];
  auto& config = elem.config.normal;
  usize fill_child_count{};
  f32 available_space =
    ui_dimension_from_axis(elem, axis) - ui_total_padding_from_axis(config, axis);
  if (config.layout_direction == layout_direction_from_axis[(usize) axis])
  {
    for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
         child_idx = layout.elements[child_idx].next_sibling)
    {
      auto& child = layout.elements[child_idx];
      if (ui_sizing_from_axis(child.config.normal, axis).type == UI_SIZING_FILL)
      {
        ++fill_child_count;
      }
      else
      {
        available_space -= ui_dimension_from_axis(child, axis);
      }
      available_space -= config.child_gap;
    }
    available_space += config.child_gap;
  }
  for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx = layout.elements[child_idx].next_sibling)
  {
    auto& child = layout.elements[child_idx];
    if (child.config.type == UI_ELEMENT_TEXT)
    {
      continue;
    }
    if (ui_sizing_from_axis(child.config.normal, axis).type == UI_SIZING_FILL)
    {
      if (config.layout_direction == layout_direction_from_axis[(usize) axis])
      {
        if (ui_sizing_from_axis(config, axis).type != UI_SIZING_FIT)
        {
          ui_dimension_from_axis(child, axis) = available_space / (f32) fill_child_count;
        }
      }
      else
      {
        ui_dimension_from_axis(child, axis) = available_space;
      }
    }
  }
}

static void ui_calculate_fill_sizing(UI_Layout& layout, UI_ElementIdx idx = 0)
{
  auto& elem = layout.elements[idx];
  if (elem.config.type == UI_ELEMENT_TEXT)
  {
    return;
  }
  ui_calculate_fill_sizing_axis(layout, idx, UI_AXIS_X);
  ui_calculate_fill_sizing_axis(layout, idx, UI_AXIS_Y);
  for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx = layout.elements[child_idx].next_sibling)
  {
    ui_calculate_fill_sizing(layout, child_idx);
  }
}

static UI_ChildAlignmentAxis
ui_child_alignment_from_axis(const UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_AXIS_X)
  {
    return config.child_alignment.x;
  }
  else if (axis == UI_AXIS_Y)
  {
    return config.child_alignment.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32& ui_pos_from_axis(UI_Element& elem, UI_Axis axis)
{
  if (axis == UI_AXIS_X)
  {
    return elem.pos.x;
  }
  else if (axis == UI_AXIS_Y)
  {
    return elem.pos.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 ui_start_padding_from_axis(UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_AXIS_X)
  {
    return config.padding.left;
  }
  else if (axis == UI_AXIS_Y)
  {
    return config.padding.top;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 ui_end_padding_from_axis(UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_AXIS_X)
  {
    return config.padding.right;
  }
  else if (axis == UI_AXIS_Y)
  {
    return config.padding.down;
  }
  ASSERT(false, "Invalid axis provided");
}

static void ui_calculate_position_axis(
  UI_Layout& layout,
  UI_ElementIdx idx,
  UI_ElementIdx child_idx,
  f32& used_space,
  UI_Axis axis
)
{
  auto& elem = layout.elements[idx];
  auto& config = elem.config.normal;
  auto& child = layout.elements[child_idx];
  if (config.layout_direction == layout_direction_from_axis[(usize) axis])
  {
    switch (ui_child_alignment_from_axis(config, axis))
    {
      case UI_CHILD_ALIGNMENT_START:
        ui_pos_from_axis(child, axis) =
          ui_pos_from_axis(elem, axis) + ui_start_padding_from_axis(config, axis) + used_space;
        used_space += ui_dimension_from_axis(child, axis) + config.child_gap;
        break;
      case UI_CHILD_ALIGNMENT_CENTER:
        ui_pos_from_axis(child, axis) =
          ui_pos_from_axis(elem, axis) + ui_start_padding_from_axis(config, axis) + used_space;
        used_space += ui_dimension_from_axis(child, axis) + config.child_gap;
        break;
      case UI_CHILD_ALIGNMENT_END:
        ui_pos_from_axis(child, axis) =
          ui_pos_from_axis(elem, axis) + ui_dimension_from_axis(elem, axis) -
          ui_end_padding_from_axis(config, axis) - ui_dimension_from_axis(child, axis) - used_space;
        used_space += ui_dimension_from_axis(child, axis) + config.child_gap;
        break;
    }
  }
  else
  {
    switch (ui_child_alignment_from_axis(config, axis))
    {
      case UI_CHILD_ALIGNMENT_START:
        ui_pos_from_axis(child, axis) =
          ui_pos_from_axis(elem, axis) + ui_start_padding_from_axis(config, axis);
        break;
      case UI_CHILD_ALIGNMENT_CENTER:
        ui_pos_from_axis(child, axis) =
          (ui_pos_from_axis(elem, axis) + ui_start_padding_from_axis(config, axis)) +
          ((ui_dimension_from_axis(elem, axis) - ui_total_padding_from_axis(config, axis)) * 0.5f) -
          (ui_dimension_from_axis(child, axis) * 0.5f);
        break;
      case UI_CHILD_ALIGNMENT_END:
        ui_pos_from_axis(child, axis) =
          ui_pos_from_axis(elem, axis) + ui_dimension_from_axis(elem, axis) -
          ui_end_padding_from_axis(config, axis) - ui_dimension_from_axis(child, axis);
        break;
    }
  }
}

static void
ui_adjust_centered_position(UI_Layout& layout, UI_ElementIdx idx, f32 used_space, UI_Axis axis)
{
  auto& elem = layout.elements[idx];
  auto& config = elem.config.normal;
  if (config.layout_direction == layout_direction_from_axis[(usize) axis] &&
      ui_child_alignment_from_axis(config, axis) == UI_CHILD_ALIGNMENT_CENTER)
  {
    f32 adjust_value = (ui_dimension_from_axis(elem, axis) - used_space) * 0.5f;
    for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
         child_idx = layout.elements[child_idx].next_sibling)
    {
      auto& child = layout.elements[child_idx];
      ui_pos_from_axis(child, axis) += adjust_value;
    }
  }
}

static void ui_calculate_positions(UI_Layout& layout, UI_ElementIdx idx = 0)
{
  auto& elem = layout.elements[idx];
  if (idx == 0)
  {
    elem.pos = {layout.pos.x, layout.pos.y};
  }
  if (elem.config.type == UI_ELEMENT_TEXT)
  {
    return;
  }
  auto& config = elem.config.normal;
  f32 used_space = 0;
  for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx = layout.elements[child_idx].next_sibling)
  {
    auto& child = layout.elements[child_idx];
    ui_calculate_position_axis(layout, idx, child_idx, used_space, UI_AXIS_X);
    ui_calculate_position_axis(layout, idx, child_idx, used_space, UI_AXIS_Y);
    if (config.scroll_value)
    {
      child.pos.y += (f32) *config.scroll_value * SCROLL_SENSITIVITY;
    }
  }
  ui_adjust_centered_position(layout, idx, used_space, UI_AXIS_X);
  ui_adjust_centered_position(layout, idx, used_space, UI_AXIS_Y);
  for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx = layout.elements[child_idx].next_sibling)
  {
    ui_calculate_positions(layout, child_idx);
  }
}

static bool ui_intersects(const vec2& point, const vec2& start, const vec2& dimensions)
{
  return (point.x > start.x && point.x < start.x + dimensions.x) &&
         (point.y > start.y && point.y < start.y + dimensions.y);
}

static bool ui_intersects(const Rectangle& a, const Rectangle& b)
{
  return a.pos.x < b.pos.x + b.dimensions.x && a.pos.x + a.dimensions.x > b.pos.x &&
         a.pos.y < b.pos.y + b.dimensions.y && a.pos.y + a.dimensions.y > b.pos.y;
}

static void ui_calculate_scroll_value(UI_Layout& layout, UI_ElementIdx idx)
{
  auto& elem = layout.elements[idx];
  auto& config = elem.config.normal;
  f32 max_height{};
  switch (config.layout_direction)
  {
    case UI_LAYOUT_DIRECTION_HORIZONTAL:
    {
      for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
           child_idx = layout.elements[child_idx].next_sibling)
      {
        auto& child = layout.elements[child_idx];
        max_height =
          std::max(max_height, child.dimensions.y + config.padding.top + config.padding.down);
      }
    }
    break;
    case UI_LAYOUT_DIRECTION_VERTICAL:
    {
      for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
           child_idx = layout.elements[child_idx].next_sibling)
      {
        auto& child = layout.elements[child_idx];
        max_height += child.dimensions.y + config.child_gap;
      }
      max_height -= config.child_gap;
      max_height += config.padding.top + config.padding.down;
    }
    break;
  }

  *config.scroll_value += layout.input->mouse_scroll;
  *config.scroll_value =
    std::clamp(*config.scroll_value, (i32) -std::floor(max_height / SCROLL_SENSITIVITY), 0);
}

static void ui_handle_scroll(UI_Layout& layout)
{
  for (i32 idx = (i32) layout.elements.size() - 1; idx >= 0; --idx)
  {
    auto& elem = layout.elements[(usize) idx];
    if (elem.config.type != UI_ELEMENT_NORMAL)
    {
      continue;
    }
    auto& config = elem.config.normal;
    if (ui_intersects(layout.input->mouse_pos, elem.pos, elem.dimensions))
    {
      if (config.scroll_value)
      {
        ui_calculate_scroll_value(layout, (UI_ElementIdx) idx);
      }
      else
      {
        for (UI_ElementIdx parent_idx = elem.parent; parent_idx != 0;
             parent_idx = layout.elements[parent_idx].parent)
        {
          auto& parent = layout.elements[parent_idx];
          if (parent.config.normal.scroll_value)
          {
            ui_calculate_scroll_value(layout, parent_idx);
            break;
          }
        }
      }
      break;
    }
  }
}

static Rectangle ui_intersection_rectangle(const Rectangle& a, const Rectangle& b)
{
  f32 left = std::max(a.pos.x, b.pos.x);
  f32 top = std::max(a.pos.y, b.pos.y);
  f32 right = std::min(a.pos.x + a.dimensions.x, b.pos.x + b.dimensions.x);
  f32 down = std::min(a.pos.y + a.dimensions.y, b.pos.y + b.dimensions.y);

  if (right > left && down > top)
  {
    return {{left, top}, {right - left, down - top}};
  }
  return {};
}

static void ui_generate_render_cmds(UI_Layout& layout, UI_ElementIdx idx = 0)
{
  // TODO: not sure where to place this line of code
  layout.elements[0].clip_rectangle = {layout.elements[0].pos, layout.elements[0].dimensions};
  auto& elem = layout.elements[idx];
  for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx = layout.elements[child_idx].next_sibling)
  {
    auto& child = layout.elements[child_idx];
    if (!ui_intersects({elem.pos, elem.dimensions}, {child.pos, child.dimensions}))
    {
      continue;
    }
    child.clip_rectangle =
      ui_intersection_rectangle({elem.pos, elem.dimensions}, elem.clip_rectangle);
    switch (child.config.type)
    {
      case UI_ELEMENT_NORMAL:
      {
        auto& config = child.config.normal;
        // TODO: this is not really render cmd generation, not sure if it belongs here
        layout.system->last_frame_data[layout.id].id_map.insert_or_assign(child.id, child_idx);
        if (config.texture)
        {
          // TODO: this is not really the ideal solution,
          // what if someone really wants to render a fully transparent texture?
          // (good enough for now tho)
          auto bg = config.bg_color != vec4{} ? config.bg_color : vec4{1, 1, 1, 1};
          layout.system->render_cmds.push_back(render_texture(
            *config.texture,
            {child.pos.x, child.pos.y, layout._z},
            child.dimensions,
            {.corner_radius = config.corner_radius,
             .tint = bg,
             .clip_rectangle = std::make_optional(child.clip_rectangle)}
          ));
        }
        else
        {
          layout.system->render_cmds.push_back(render_quad(
            {child.pos.x, child.pos.y, layout._z},
            child.dimensions,
            {.corner_radius = config.corner_radius,
             .tint = config.bg_color,
             .clip_rectangle = std::make_optional(child.clip_rectangle)}
          ));
        }
      }
      break;
      case UI_ELEMENT_TEXT:
      {
        auto& config = child.config.text;
        auto& str = layout.strings[config.string_idx];
        for (usize i = 0; i < str.size(); ++i)
        {
          vec2 texture_offset{};
          if (std::islower(str[i]))
          {
            texture_offset = {(f32) (str[i] - 'a'), 1};
          }
          else if (std::isupper(str[i]))
          {
            texture_offset = {(f32) (str[i] - 'A'), 1};
          }
          else if (std::isdigit(str[i]))
          {
            texture_offset = {(f32) (str[i] - '0'), 2};
          }
          else if (str[i] >= '!' && str[i] <= '.')
          {
            texture_offset = {(f32) (str[i] - '!'), 3};
          }
          else if (str[i] >= ':' && str[i] <= ';')
          {
            texture_offset = {(f32) (str[i] - ':'), 4};
          }
          else if (str[i] >= '[' && str[i] <= '_')
          {
            texture_offset = {(f32) (str[i] - '['), 5};
          }
          else if (std::isspace(str[i]))
          {
            continue;
          }
          else
          {
            ASSERT(false, "Unsupported character '{}'({}) found in drawing", str[i], (i32) str[i]);
          }
          layout.system->render_cmds.push_back(render_texture_part(
            layout.font_texture,
            {child.pos.x + ((f32) i * layout.char_size.x * config.size), child.pos.y, layout._z},
            layout.char_size * config.size,
            layout.char_size * texture_offset,
            layout.char_size,
            {.corner_radius = 0.0f,
             .tint = {1, 1, 1, 1},
             .clip_rectangle = std::make_optional(child.clip_rectangle)}
          ));
        }
      }
      break;
    }
    layout._z += 0.001f;
    ui_generate_render_cmds(layout, child_idx);
  }
}

void ui_layout_end(UI_Layout& layout)
{
  ui_element_end(
    layout,
    {.sizing =
       {ui_sizing_fixed((u16) layout.max_dimensions.x),
        ui_sizing_fixed((u16) layout.max_dimensions.y)}}
  );
  ui_calculate_text_fit_fixed_sizing(layout);
  ui_calculate_fill_sizing(layout);
  ui_calculate_positions(layout);
  ui_handle_scroll(layout);
  layout.system->last_frame_data[layout.id].id_map.clear();
  ui_generate_render_cmds(layout);
  layout.system->last_frame_data[layout.id].elements = layout.elements;
}

// TODO: i hate this, but currently i dont have a clue what could be better
static void set_first_child_or_next_sibling(UI_Layout& layout)
{
  auto& parent = layout.elements[layout._active_parent];
  if (parent.first_child == 0)
  {
    parent.first_child = layout.elements.size() - 1;
  }
  else
  {
    UI_Element* prev_sibling{};
    for (UI_ElementIdx idx = parent.first_child; idx != 0; idx = layout.elements[idx].next_sibling)
    {
      prev_sibling = &layout.elements[idx];
    }
    prev_sibling->next_sibling = layout.elements.size() - 1;
  }
}

void ui_element_begin(UI_Layout& layout, UI_Id id, const UI_StateOptions& state_options)
{
  UI_IdInternal id_internal =
    id ? std::hash<UI_Id>{}(id) : std::hash<UI_ElementIdx>{}(layout.elements.size());
  if (layout.system->last_frame_data[layout.id].id_map.contains(id_internal))
  {
    auto& idx = layout.system->last_frame_data[layout.id].id_map[id_internal];
    auto& elem = layout.system->last_frame_data[layout.id].elements[idx];
    Rectangle interaction_rect =
      ui_intersection_rectangle({elem.pos, elem.dimensions}, elem.clip_rectangle);
    bool hovered =
      ui_intersects(layout.input->mouse_pos, interaction_rect.pos, interaction_rect.dimensions);
    bool nothing_on_top = true;
    if (hovered)
    {
      for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
           child_idx = layout.system->last_frame_data[layout.id].elements[child_idx].next_sibling)
      {
        auto& child = layout.system->last_frame_data[layout.id].elements[child_idx];
        if (child.config.type != UI_ELEMENT_NORMAL)
        {
          continue;
        }
        auto child_interaction_rect =
          ui_intersection_rectangle({child.pos, child.dimensions}, child.clip_rectangle);
        if (ui_intersects(
              layout.input->mouse_pos,
              child_interaction_rect.pos,
              child_interaction_rect.dimensions
            ))
        {
          nothing_on_top = false;
        }
      }
    }
    if (nothing_on_top)
    {
      if (state_options.hovered)
      {
        *state_options.hovered = hovered;
      }
      if (state_options.clicked)
      {
        *state_options.clicked = hovered && os_key_just_pressed(layout.input->lmb);
      }
    }
  }
  layout.elements.push_back({
    .id = id_internal,
    .parent = layout._active_parent,
    .config = {.type = UI_ELEMENT_NORMAL},
  });
  set_first_child_or_next_sibling(layout);
  layout._active_parent = layout.elements.size() - 1;
}

void ui_element_end(UI_Layout& layout, const UI_ElementConfigNormal& config)
{
  auto& elem = layout.elements[layout._active_parent];
  ASSERT(elem.config.type == UI_ELEMENT_NORMAL, "Cannot close a not normal element");
  elem.config.normal = config;
  layout._active_parent = layout.elements[layout._active_parent].parent;
}

void ui_text(UI_Layout& layout, std::string_view text, f32 size)
{
  layout.strings.push_back(std::string{text});
  layout.elements.push_back({
    .parent = layout._active_parent,
    .config =
      {.type = UI_ELEMENT_TEXT, .text = {.string_idx = layout.strings.size() - 1, .size = size}},
  });
  set_first_child_or_next_sibling(layout);
}
