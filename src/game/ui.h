#ifndef GAME_UI_H
#define GAME_UI_H

#include <vector>

#include "base/math.h"
#include "os/os.h"
#include "renderer.h"

// NOTE: possibly im doing way too many iterations over the whole data set
// so if ever performance becomes an issue i could look at that

enum UI_LayoutDirection
{
  UI_LAYOUT_DIRECTION_HORIZONTAL,
  UI_LAYOUT_DIRECTION_VERTICAL,
};

enum UI_SizingType
{
  UI_SIZING_FIT,
  UI_SIZING_FILL,
  UI_SIZING_FIXED,
};

struct UI_SizingAxis
{
  UI_SizingType type{};
  u16 fixed_px{};
};

inline UI_SizingAxis ui_sizing_fit()
{
  return {.type = UI_SIZING_FIT};
}

inline UI_SizingAxis ui_sizing_fill()
{
  return {.type = UI_SIZING_FILL};
}

inline UI_SizingAxis ui_sizing_fixed(u16 px)
{
  return {.type = UI_SIZING_FIXED, .fixed_px = px};
}

struct UI_Sizing
{
  UI_SizingAxis width{};
  UI_SizingAxis height{};
};

struct UI_Padding
{
  u16 top{};
  u16 down{};
  u16 left{};
  u16 right{};
};

inline UI_Padding ui_padding_all(u16 value)
{
  return {value, value, value, value};
}

enum UI_ChildAlignmentAxis
{
  UI_CHILD_ALIGNMENT_START,
  UI_CHILD_ALIGNMENT_CENTER,
  UI_CHILD_ALIGNMENT_END,
};

struct UI_ChildAlignment
{
  UI_ChildAlignmentAxis x{};
  UI_ChildAlignmentAxis y{};
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

enum UI_ElementType
{
  UI_ELEMENT_NORMAL,
  UI_ELEMENT_TEXT,
};

struct UI_ElementConfig
{
  UI_ElementType type{};
  union
  {
    UI_ElementConfigNormal normal{};
    struct
    {
      usize string_idx{};
      f32 size{};
    } text;
  };
};

struct UI_StateOptions
{
  bool* clicked{};
  bool* hovered{};
};

using UI_ElementIdx = usize;
using UI_Id = const char*;
using UI_IdInternal = usize;

struct UI_Element
{
  UI_IdInternal id{};
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

struct UI_System
{
  std::vector<Render_Cmd2D> render_cmds{};
  struct LastFrameData
  {
    std::unordered_map<UI_IdInternal, UI_ElementIdx> id_map{};
    std::vector<UI_Element> elements{};
  };
  std::unordered_map<UI_IdInternal, LastFrameData> last_frame_data{};
};

// NOTE: needs to be called once every frame, before the first ui_begin_layout
void ui_system_update(UI_System& system);

struct UI_Layout
{
  UI_IdInternal id{};
  UI_System* system{};
  const OS_Input* input{};
  AssetStore* assets{};
  // NOTE: weird hack to get the union working, because c++
  std::vector<std::string> strings{};
  std::vector<UI_Element> elements{};
  vec3 pos{};
  vec2 max_dimensions{};

  vec2 char_size{};
  TextureHandle font_texture{};

  bool _element{};
  UI_ElementIdx _active_parent{};
  f32 _z{};
};

bool ui_intersects(const vec2& point, const vec2& start, const vec2& dimensions);

// TODO: sometimes i dont want to pass max_dimensions
UI_Layout ui_layout_begin(
  UI_Id id,
  UI_System& system,
  const OS_Input& input,
  AssetStore& assets,
  const vec3& pos,
  const vec2& max_dimensions,
  const vec2& char_size,
  TextureHandle font_texture
);
void ui_layout_end(UI_Layout& layout);

#define UI_AUTO_ID nullptr
void ui_element_begin(UI_Layout& layout, UI_Id id, const UI_StateOptions& state_options = {});
void ui_element_end(UI_Layout& layout, const UI_ElementConfigNormal& config = {});

void ui_text(UI_Layout& layout, std::string_view text, f32 size);

#endif
