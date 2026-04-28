#include "ui.h"

#include <cstdlib>
#include <optional>
#include <cmath>

#include "base/base.h"
#include "game/renderer.h"

UI_Layout ui_begin_layout(
  const os::Input& input,
  const vec3& pos,
  const vec2& max_dimensions,
  const vec2& char_size,
  TextureHandle font_texture
)
{
  UI_Layout layout = {
    .input = input,
    .pos = pos,
    .char_size = char_size,
    .font_texture = font_texture,
  };
  ui_begin_element(
    layout,
    {.sizing = {
       UI_SizingAxis::fixed((u16) max_dimensions.x),
       UI_SizingAxis::fixed((u16) max_dimensions.y),
     }}
  );
  return layout;
}

enum class UI_Axis
{
  X,
  Y,
};

constexpr static std::array<UI_LayoutDirection, 2> layout_direction_from_axis = []()
{
  std::array<UI_LayoutDirection, 2> t{};
  t[(usize) UI_Axis::X] = UI_LayoutDirection::HORIZONTAL;
  t[(usize) UI_Axis::Y] = UI_LayoutDirection::VERTICAL;
  return t;
}();

constexpr static std::array<UI_Axis, 2> axis_from_layout_direction = []()
{
  std::array<UI_Axis, 2> t{};
  t[(usize) UI_LayoutDirection::HORIZONTAL] = UI_Axis::X;
  t[(usize) UI_LayoutDirection::VERTICAL] = UI_Axis::Y;
  return t;
}();

static UI_SizingAxis& ui_sizing_from_axis(UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_Axis::X)
  {
    return config.sizing.width;
  }
  else if (axis == UI_Axis::Y)
  {
    return config.sizing.height;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32& ui_dimension_from_axis(UI_Element& elem, UI_Axis axis)
{
  if (axis == UI_Axis::X)
  {
    return elem.dimensions.x;
  }
  else if (axis == UI_Axis::Y)
  {
    return elem.dimensions.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 ui_total_padding_from_axis(UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_Axis::X)
  {
    return config.padding.left + config.padding.right;
  }
  else if (axis == UI_Axis::Y)
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
    case UI_SizingType::FIXED:
      ui_dimension_from_axis(elem, axis) = sizing.fixed_px;
      break;
    case UI_SizingType::FIT:
    case UI_SizingType::FILL:
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

// NOTE: here im also calculating fit sizing dimensions for fill sizing
// because if one element has a fill sizing and the parent has a fit sizing
// the fill sizing child will just be treated as a fit sizing instead
// UNLESS the layout direction is of the opposite axis (vertical for x, horizontal for y)
// then this calculation is just thrown away, and the maximum free parent space is assigned to that
// axis
// dont know if that is the 'correct' behavior, but that is what im going with for now
// the other option is to make the fit sizing parent behave like a fill sizing instead
// but in my head the fit sizing is supposed to occupy the smallest possible amount of space
// so the current approach makes more sense
// maybe there is some other more 'correct' approach that i cant think of rn idk
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
    case UI_ElementType::NORMAL:
    {
      ui_calculate_fit_fixed_sizing_axis(layout, idx, UI_Axis::X);
      ui_calculate_fit_fixed_sizing_axis(layout, idx, UI_Axis::Y);
      auto& config = elem.config.normal;
      if (has_children)
      {
        ui_dimension_from_axis(elem, axis_from_layout_direction[(usize) config.layout_direction]) -=
          config.child_gap;
      }
    }
    break;

    case UI_ElementType::TEXT:
    {
      auto& config = elem.config.text;
      elem.dimensions =
        vec2{layout.char_size.x * (f32) config.text.size(), layout.char_size.y} * config.size;
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
      if (ui_sizing_from_axis(child.config.normal, axis).type == UI_SizingType::FILL)
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
    if (child.config.type == UI_ElementType::TEXT)
    {
      continue;
    }
    if (ui_sizing_from_axis(child.config.normal, axis).type == UI_SizingType::FILL)
    {
      if (config.layout_direction == layout_direction_from_axis[(usize) axis])
      {
        if (ui_sizing_from_axis(config, axis).type != UI_SizingType::FIT)
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
  if (elem.config.type == UI_ElementType::TEXT)
  {
    return;
  }
  ui_calculate_fill_sizing_axis(layout, idx, UI_Axis::X);
  ui_calculate_fill_sizing_axis(layout, idx, UI_Axis::Y);
  for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx = layout.elements[child_idx].next_sibling)
  {
    ui_calculate_fill_sizing(layout, child_idx);
  }
}

static UI_ChildAlignmentAxis
ui_child_alignment_from_axis(const UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_Axis::X)
  {
    return config.child_alignment.x;
  }
  else if (axis == UI_Axis::Y)
  {
    return config.child_alignment.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32& ui_pos_from_axis(UI_Element& elem, UI_Axis axis)
{
  if (axis == UI_Axis::X)
  {
    return elem.pos.x;
  }
  else if (axis == UI_Axis::Y)
  {
    return elem.pos.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 ui_start_padding_from_axis(UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_Axis::X)
  {
    return config.padding.left;
  }
  else if (axis == UI_Axis::Y)
  {
    return config.padding.top;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 ui_end_padding_from_axis(UI_ElementConfigNormal& config, UI_Axis axis)
{
  if (axis == UI_Axis::X)
  {
    return config.padding.right;
  }
  else if (axis == UI_Axis::Y)
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
      case UI_ChildAlignmentAxis::START:
        ui_pos_from_axis(child, axis) =
          ui_pos_from_axis(elem, axis) + ui_start_padding_from_axis(config, axis) + used_space;
        used_space += ui_dimension_from_axis(child, axis) + config.child_gap;
        break;
      case UI_ChildAlignmentAxis::CENTER:
        ui_pos_from_axis(child, axis) =
          ui_pos_from_axis(elem, axis) + ui_start_padding_from_axis(config, axis) + used_space;
        used_space += ui_dimension_from_axis(child, axis) + config.child_gap;
        break;
      case UI_ChildAlignmentAxis::END:
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
      case UI_ChildAlignmentAxis::START:
        ui_pos_from_axis(child, axis) =
          ui_pos_from_axis(elem, axis) + ui_start_padding_from_axis(config, axis);
        break;
      case UI_ChildAlignmentAxis::CENTER:
        ui_pos_from_axis(child, axis) =
          (ui_pos_from_axis(elem, axis) + ui_start_padding_from_axis(config, axis)) +
          ((ui_dimension_from_axis(elem, axis) - ui_total_padding_from_axis(config, axis)) * 0.5f) -
          (ui_dimension_from_axis(child, axis) * 0.5f);
        break;
      case UI_ChildAlignmentAxis::END:
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
  if (
    config.layout_direction == layout_direction_from_axis[(usize) axis] &&
    ui_child_alignment_from_axis(config, axis) == UI_ChildAlignmentAxis::CENTER
  )
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
  if (elem.config.type == UI_ElementType::TEXT)
  {
    return;
  }
  auto& config = elem.config.normal;
  f32 used_space = 0;
  for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx = layout.elements[child_idx].next_sibling)
  {
    auto& child = layout.elements[child_idx];
    ui_calculate_position_axis(layout, idx, child_idx, used_space, UI_Axis::X);
    ui_calculate_position_axis(layout, idx, child_idx, used_space, UI_Axis::Y);
    if (config.scroll_value)
    {
      child.pos.y += (f32) *config.scroll_value * SCROLL_SENSITIVITY;
    }
  }
  ui_adjust_centered_position(layout, idx, used_space, UI_Axis::X);
  ui_adjust_centered_position(layout, idx, used_space, UI_Axis::Y);
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

static void ui_check_mouse_interactions(UI_Layout& layout)
{
  for (i32 idx = (i32) layout.elements.size() - 1; idx >= 0; --idx)
  {
    auto& elem = layout.elements[(usize) idx];
    if (elem.config.type != UI_ElementType::NORMAL)
    {
      continue;
    }
    auto& config = elem.config.normal;
    bool hovered = ui_intersects(layout.input.mouse_pos, elem.pos, elem.dimensions);
    // TODO: this is still just wrong
    if (config.clicked)
    {
      *config.clicked = hovered && layout.input.lmb.just_pressed();
      break;
    }
    if (config.hovered)
    {
      *config.hovered = hovered;
      break;
    }
    if (config.scroll_value)
    {
      f32 max_height{};
      for (UI_ElementIdx child_idx = elem.first_child; child_idx != 0;
           child_idx = layout.elements[child_idx].next_sibling)
      {
        auto& child = layout.elements[child_idx];
        switch (config.layout_direction)
        {
          case UI_LayoutDirection::HORIZONTAL:
            max_height = std::max(max_height, child.dimensions.y);
            break;
          case UI_LayoutDirection::VERTICAL:
            max_height += child.dimensions.y + config.child_gap;
            break;
        }
      }
      if (config.layout_direction == UI_LayoutDirection::VERTICAL)
      {
        max_height -= config.child_gap;
      }
      *config.scroll_value += layout.input.mouse_scroll;
      *config.scroll_value =
        std::clamp(*config.scroll_value, (i32) -std::floor(max_height / SCROLL_SENSITIVITY), 0);
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

static void ui_generate_render_cmds(UI_Layout& layout, std::vector<render::Cmd2D>& render_cmds)
{
  layout.elements[0].clip_rectangle = {layout.elements[0].pos, layout.elements[0].dimensions};
  // TODO: iterate by children instead?
  for (UI_ElementIdx idx = 1; idx < layout.elements.size(); ++idx)
  {
    auto& elem = layout.elements[idx];
    auto& parent = layout.elements[elem.parent];
    elem.clip_rectangle =
      ui_intersection_rectangle({parent.pos, parent.dimensions}, parent.clip_rectangle);
    switch (elem.config.type)
    {
      case UI_ElementType::NORMAL:
      {
        auto& config = elem.config.normal;
        if (config.texture)
        {
          // TODO: this is not really the ideal solution,
          // what if someone really wants to render a fully transparent texture?
          // (good enough for now tho)
          auto bg = config.bg_color != vec4{} ? config.bg_color : vec4{1, 1, 1, 1};
          render_cmds.push_back(
            render::texture(
              *config.texture,
              {elem.pos.x, elem.pos.y, layout._z},
              elem.dimensions,
              {.corner_radius = config.corner_radius,
               .tint = bg,
               .clip_rectangle = std::make_optional(elem.clip_rectangle)}
            )
          );
        }
        else
        {
          render_cmds.push_back(
            render::quad(
              {elem.pos.x, elem.pos.y, layout._z},
              elem.dimensions,
              {.corner_radius = config.corner_radius,
               .tint = config.bg_color,
               .clip_rectangle = std::make_optional(elem.clip_rectangle)}
            )
          );
        }
      }
      break;
      case UI_ElementType::TEXT:
      {
        auto& config = elem.config.text;
        std::vector<uvec2> text_parts{};
        text_parts.resize(config.text.size());
        for (usize i = 0; i < config.text.size(); ++i)
        {
          if (std::islower(config.text[i]))
          {
            text_parts[i] = {(u32) (config.text[i] - 'a'), 1};
          }
          else if (std::isupper(config.text[i]))
          {
            text_parts[i] = {(u32) (config.text[i] - 'A'), 1};
          }
          else if (std::isdigit(config.text[i]))
          {
            text_parts[i] = {(u32) (config.text[i] - '0'), 2};
          }
          else if (std::isspace(config.text[i]))
          {
          }
          else if (config.text[i] >= '!' && config.text[i] <= '-')
          {
            text_parts[i] = {(u32) (config.text[i] - '!'), 3};
          }
          else
          {
            ASSERT(false, "Unsupported character '{}' found in drawing", config.text[i]);
          }
        }

        for (usize i = 0; i < text_parts.size(); ++i)
        {
          if (text_parts[i].y == 0)
          {
            continue;
          }
          render_cmds.push_back(
            render::texture_part(
              layout.font_texture,
              {elem.pos.x + ((f32) i * layout.char_size.x * config.size), elem.pos.y, layout._z},
              layout.char_size * config.size,
              layout.char_size * vec2{(f32) text_parts[i].x, (f32) text_parts[i].y},
              layout.char_size,
              {.corner_radius = 0.0f,
               .tint = {1, 1, 1, 1},
               .clip_rectangle = std::make_optional(elem.clip_rectangle)}
            )
          );
        }
      }
      break;
    }
    layout._z += 0.001f;
  }
}

std::vector<render::Cmd2D> ui_end_layout(UI_Layout& layout)
{
  ui_end_element(layout);
  ui_calculate_text_fit_fixed_sizing(layout);
  ui_calculate_fill_sizing(layout);
  ui_calculate_positions(layout);
  ui_check_mouse_interactions(layout);
  std::vector<render::Cmd2D> render_cmds{};
  ui_generate_render_cmds(layout, render_cmds);
  return render_cmds;
}

static void set_first_child_or_next_sibling(UI_Layout& layout)
{
  auto& parent = layout.elements[layout._active_parent];
  // TODO: i hate this, but currently i dont have a clue what could be better
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

void ui_begin_element(UI_Layout& layout, const UI_ElementConfigNormal& config)
{
  layout.elements.push_back({
    .parent = layout._active_parent,
    .config = {.type = UI_ElementType::NORMAL, .normal = config},
  });
  set_first_child_or_next_sibling(layout);
  layout._active_parent = layout.elements.size() - 1;
}

void ui_end_element(UI_Layout& layout)
{
  layout._active_parent = layout.elements[layout._active_parent].parent;
}

void ui_text(UI_Layout& layout, std::string_view text, f32 size)
{
  layout.elements.push_back({
    .parent = layout._active_parent,
    .config = {.type = UI_ElementType::TEXT, .text = {.text = text, .size = size}},
  });
  set_first_child_or_next_sibling(layout);
}
