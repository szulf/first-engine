#pragma once

#include <vector>

#include "base/math.h"
#include "os/os.h"
#include "renderer.h"

// NOTE: possibly im doing way too many iterations over the whole data set
// so if ever performance becomes an issue i could look at that

enum class UI_LayoutDirection
{
  HORIZONTAL,
  VERTICAL,
};

enum class UI_SizingType
{
  FIT,
  FILL,
  FIXED,
};

struct UI_SizingAxis
{
  UI_SizingType type{};
  u16 fixed_px{};

  static UI_SizingAxis fit()
  {
    return {.type = UI_SizingType::FIT};
  }
  static UI_SizingAxis fill()
  {
    return {.type = UI_SizingType::FILL};
  }
  static UI_SizingAxis fixed(u16 px)
  {
    return {.type = UI_SizingType::FIXED, .fixed_px = px};
  }
};

struct UI_Sizing
{
  UI_SizingAxis width{};
  UI_SizingAxis height{};
};

struct UI_Padding
{
  u16 top;
  u16 down;
  u16 left;
  u16 right;

  static UI_Padding all(u16 value)
  {
    return {value, value, value, value};
  }
};

enum class UI_ChildAlignmentAxis
{
  START,
  CENTER,
  END,
};

struct UI_ChildAlignment
{
  UI_ChildAlignmentAxis x;
  UI_ChildAlignmentAxis y;
};

#define SCROLL_SENSITIVITY 50

struct UI_ElementConfigNormal
{
  UI_LayoutDirection layout_direction{};
  UI_Sizing sizing{};
  UI_Padding padding{};
  u16 child_gap{};
  UI_ChildAlignment child_alignment{};
  vec4 bg_color{};
  std::optional<TextureHandle> texture{};
  // TODO: should i have a separate corner_radius for each of the corners? (probably yes)
  f32 corner_radius{};
  i32* scroll_value{};
};

enum class UI_ElementType
{
  NORMAL,
  TEXT,
};

struct UI_ElementConfig
{
  UI_ElementType type{};
  union
  {
    UI_ElementConfigNormal normal;
    struct
    {
      std::string_view text{};
      f32 size{};
    } text;
  };
};

using UI_ElementId = const char*;
using UI_ElementIdInternal = usize;

struct UI_System
{
  // NOTE: this is the intersection of the elements rectangle and its clip rectangle
  std::unordered_map<UI_ElementIdInternal, Rectangle> last_frame_states{};
  std::vector<render::Cmd2D> render_cmds{};
};

struct UI_StateOptions
{
  bool* clicked{};
  bool* hovered{};
};

using UI_ElementIdx = usize;

struct UI_Element
{
  UI_ElementIdInternal id{};
  UI_ElementIdx parent{};
  // NOTE: if idx == 0 then there is no child
  UI_ElementIdx first_child{};
  // NOTE: if idx == 0 then there is no sibling
  UI_ElementIdx next_sibling{};
  UI_ElementConfig config{};
  vec2 dimensions{};
  vec2 pos{};
  Rectangle clip_rectangle{};
};

struct UI_Layout
{
  UI_System& system;
  const os::Input& input;
  std::vector<UI_Element> elements{};
  vec3 pos{};
  vec2 max_dimensions{};

  vec2 char_size{};
  TextureHandle font_texture{};

  bool _element{};
  UI_ElementIdx _active_parent{};
  f32 _z{};
};

// TODO: sometimes i dont want to pass max_dimensions
UI_Layout ui_begin_layout(
  UI_System& system,
  const os::Input& input,
  const vec3& pos,
  const vec2& max_dimensions,
  const vec2& char_size,
  TextureHandle font_texture
);
void ui_end_layout(UI_Layout& layout);

#define UI_AUTO_ID nullptr
void ui_begin_element(
  UI_Layout& layout,
  UI_ElementId id,
  const UI_StateOptions& state_options = {}
);
void ui_end_element(UI_Layout& layout, const UI_ElementConfigNormal& config = {});

void ui_text(UI_Layout& layout, std::string_view text, f32 size);
