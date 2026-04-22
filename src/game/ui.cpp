#include "ui.h"
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

// NOTE: here im also calculating fit sizing dimensions for fill sizing
// because if one element has a fill sizing and the parent has a fit sizing
// the fill sizing child will just be treated as a fit sizing instead
// dont know if that is the 'correct' behavior, but that is what im going with for now
// the other option is to make the fit sizing parent behave like a fill sizing instead
// but in my head the fit sizing is supposed to occupy the smallest possible amount of space
// so the current approach makes more sense
// maybe there is some other more 'correct' approach that i cant think of rn idk
static void ui_calculate_text_fit_fixed_sizing(UI_Layout& layout, UI_ElementIdx idx = 0)
{
  for (UI_ElementIdx i = idx + 1; i < layout.elements.size(); ++i)
  {
    auto& child = layout.elements[i];
    if (child.parent < idx)
    {
      break;
    }
    if (child.parent != idx)
    {
      continue;
    }
    ui_calculate_text_fit_fixed_sizing(layout, i);
  }

  auto& elem = layout.elements[idx];
  if (elem.config.type == UI_ElementType::NORMAL)
  {
    auto& config = elem.config.normal;
    // TODO: better name and make it an actual function not a lambda?
    auto calculation = [&layout, &config, &idx](f32& dimension, UI_SizingAxis sizing, bool width)
    {
      if (sizing.type == UI_SizingType::FIXED)
      {
        dimension = sizing.fixed_px;
      }
      else if (sizing.type == UI_SizingType::FIT || sizing.type == UI_SizingType::FILL)
      {
        for (UI_ElementIdx child_idx = idx + 1; child_idx < layout.elements.size(); ++child_idx)
        {
          auto& child = layout.elements[child_idx];
          if (child.parent < idx)
          {
            break;
          }
          if (child.parent != idx)
          {
            continue;
          }
          switch (config.layout_direction)
          {
            case UI_LayoutDirection::HORIZONTAL:
              if (width)
              {
                dimension += child.dimensions.x;
              }
              else
              {
                dimension = std::max(dimension, child.dimensions.y);
              }
              break;
            case UI_LayoutDirection::VERTICAL:
              if (width)
              {
                dimension = std::max(dimension, child.dimensions.x);
              }
              else
              {
                dimension += child.dimensions.y;
              }
              break;
          }
        }
      }
    };
    calculation(elem.dimensions.x, config.sizing.width, true);
    calculation(elem.dimensions.y, config.sizing.height, false);
  }
  else if (elem.config.type == UI_ElementType::TEXT)
  {
    auto& config = elem.config.text;
    elem.dimensions =
      vec2{layout.char_size.x * (f32) config.text.size(), layout.char_size.y} * config.size;
  }
}

// NOTE: calculating the dimensions for fill sizing for all the children then recursing into them
static void ui_calculate_fill_sizing(UI_Layout& layout, UI_ElementIdx idx = 0)
{
  auto& elem = layout.elements[idx];
  if (elem.config.type == UI_ElementType::TEXT)
  {
    return;
  }
  auto& config = elem.config.normal;
  // TODO: can i somehow collapse these width and height into a single code path?
  // but in a better way than in ui_calculate_text_fit_fixed_sizing()
  // NOTE: width
  if (config.sizing.width.type != UI_SizingType::FIT)
  {
    usize fill_child_count{};
    f32 available_space = elem.dimensions.x;
    for (UI_ElementIdx child_idx = idx + 1; child_idx < layout.elements.size(); ++child_idx)
    {
      auto& child = layout.elements[child_idx];
      if (child.parent < idx)
      {
        break;
      }
      if (child.parent != idx || child.config.type == UI_ElementType::TEXT)
      {
        continue;
      }
      if (child.config.normal.sizing.width.type == UI_SizingType::FILL)
      {
        ++fill_child_count;
      }
      else
      {
        available_space -= child.dimensions.x;
      }
    }
    for (UI_ElementIdx child_idx = idx + 1; child_idx < layout.elements.size(); ++child_idx)
    {
      auto& child = layout.elements[child_idx];
      if (child.parent < idx)
      {
        break;
      }
      if (child.parent != idx || child.config.type == UI_ElementType::TEXT)
      {
        continue;
      }
      if (child.config.normal.sizing.width.type == UI_SizingType::FILL)
      {
        switch (config.layout_direction)
        {
          case UI_LayoutDirection::HORIZONTAL:
            child.dimensions.x = available_space / (f32) fill_child_count;
            break;
          case UI_LayoutDirection::VERTICAL:
            child.dimensions.x = available_space;
            break;
        }
      }
    }
  }
  // NOTE: height
  if (config.sizing.height.type != UI_SizingType::FIT)
  {
    usize fill_child_count{};
    f32 available_space = elem.dimensions.y;
    for (UI_ElementIdx child_idx = idx + 1; child_idx < layout.elements.size(); ++child_idx)
    {
      auto& child = layout.elements[child_idx];
      if (child.parent < idx)
      {
        break;
      }
      if (child.parent != idx || child.config.type == UI_ElementType::TEXT)
      {
        continue;
      }
      if (child.config.normal.sizing.height.type == UI_SizingType::FILL)
      {
        ++fill_child_count;
      }
      else
      {
        available_space -= child.dimensions.y;
      }
    }
    for (UI_ElementIdx child_idx = idx + 1; child_idx < layout.elements.size(); ++child_idx)
    {
      auto& child = layout.elements[child_idx];
      if (child.parent < idx)
      {
        break;
      }
      if (child.parent != idx || child.config.type == UI_ElementType::TEXT)
      {
        continue;
      }
      if (child.config.normal.sizing.height.type == UI_SizingType::FILL)
      {
        switch (config.layout_direction)
        {
          case UI_LayoutDirection::HORIZONTAL:
            child.dimensions.y = available_space;
            break;
          case UI_LayoutDirection::VERTICAL:
            child.dimensions.y = available_space / (f32) fill_child_count;
            break;
        }
      }
    }
  }

  for (UI_ElementIdx child_idx = idx + 1; child_idx < layout.elements.size(); ++child_idx)
  {
    auto& child = layout.elements[child_idx];
    if (child.parent < idx)
    {
      break;
    }
    if (child.parent != idx)
    {
      continue;
    }
    ui_calculate_fill_sizing(layout, child_idx);
  }
}

static void ui_calculate_positions(UI_Layout& layout, UI_ElementIdx idx = 0)
{
  auto& elem = layout.elements[idx];
  if (idx == 0)
  {
    elem.pos = {layout.pos.x, layout.pos.y};
  }
  f32 used_space = 0;
  for (UI_ElementIdx child_idx = idx + 1; child_idx < layout.elements.size(); ++child_idx)
  {
    auto& child = layout.elements[child_idx];
    if (child.parent < idx)
    {
      break;
    }
    if (child.parent != idx)
    {
      continue;
    }
    ASSERT(elem.config.type == UI_ElementType::NORMAL, "text elements dont have children");
    auto& config = elem.config.normal;
    switch (config.layout_direction)
    {
      case UI_LayoutDirection::HORIZONTAL:
        child.pos = {elem.pos.x + used_space, elem.pos.y};
        used_space += child.dimensions.x;
        break;
      case UI_LayoutDirection::VERTICAL:
        child.pos = {elem.pos.x, elem.pos.y + used_space};
        used_space += child.dimensions.y;
        break;
    }
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
  for (const auto& elem : layout.elements)
  {
    if (elem.config.type != UI_ElementType::NORMAL)
    {
      continue;
    }
    auto& config = elem.config.normal;
    bool hovered = ui_intersects(layout.input.mouse_pos, elem.pos, elem.dimensions);
    if (config.clicked)
    {
      *config.clicked = hovered && layout.input.lmb.just_pressed();
    }
    if (config.hovered)
    {
      *config.hovered = hovered;
    }
  }
}

static void ui_generate_render_cmds(UI_Layout& layout, std::vector<render::Cmd2D>& render_cmds)
{
  for (const auto& elem : layout.elements)
  {
    switch (elem.config.type)
    {
      case UI_ElementType::NORMAL:
      {
        auto& config = elem.config.normal;
        render_cmds.push_back(
          render::quad({elem.pos.x, elem.pos.y, layout._z}, elem.dimensions, config.bg_color)
        );
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
              {1, 1, 1, 1}
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
  // TODO: inline these?
  ui_calculate_text_fit_fixed_sizing(layout);
  ui_calculate_fill_sizing(layout);
  ui_calculate_positions(layout);
  ui_check_mouse_interactions(layout);
  std::vector<render::Cmd2D> render_cmds{};
  ui_generate_render_cmds(layout, render_cmds);
  return render_cmds;
}

void ui_begin_element(UI_Layout& layout, const UI_ElementConfigNormal& config)
{
  layout.elements.push_back({
    .config = {.type = UI_ElementType::NORMAL, .normal = config},
    .parent = layout._active_parent,
  });
  layout._active_parent = layout.elements.size() - 1;
}

void ui_end_element(UI_Layout& layout)
{
  layout._active_parent = layout.elements[layout._active_parent].parent;
}

void ui_text(UI_Layout& layout, std::string_view text, f32 size)
{
  layout.elements.push_back({
    .config = {.type = UI_ElementType::TEXT, .text = {.text = text, .size = size}},
    .parent = layout._active_parent,
  });
}
