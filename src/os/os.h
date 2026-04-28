#ifndef OS_H
#define OS_H

#include <expected>
#include <memory>
#include <string>
#include <string_view>

#include "base/base.h"
#include "base/math.h"
#include "base/enum_array.h"

namespace os
{

void init();
void shutdown();

enum class Key
{
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  SPACE,
  LSHIFT,

  COUNT
};

std::expected<std::string_view, std::string_view> key_to_string(Key key);
std::expected<Key, std::string_view> string_to_key(std::string_view str);

struct KeyState
{
  inline constexpr bool just_pressed() const
  {
    return down && transition_count != 0;
  }

  u32 transition_count;
  bool down;
};

struct Input
{
  void clear();

  inline constexpr KeyState& key(Key key)
  {
    return keys[key];
  }

  EnumArray<Key, KeyState> keys;
  KeyState lmb;
  KeyState rmb;

  i32 mouse_scroll;
  vec2 mouse_pos;
  vec2 mouse_delta;
};

class Window
{
public:
  struct WindowData
  {
    virtual ~WindowData() {}
  };

public:
  Window(std::string_view name, uvec2 dimensions);
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&& other);
  Window& operator=(Window&& other);
  ~Window();

  void update();
  void swap_buffers();
  void hide_mouse_pointer();
  void show_mouse_pointer();

  [[nodiscard]] inline constexpr bool running() const noexcept
  {
    return m_running;
  }
  [[nodiscard]] inline constexpr Input& input() noexcept
  {
    return m_input;
  }
  [[nodiscard]] inline constexpr u32 width() const noexcept
  {
    return m_dimensions.x;
  }
  [[nodiscard]] inline constexpr u32 height() const noexcept
  {
    return m_dimensions.y;
  }
  [[nodiscard]] inline constexpr uvec2 dimensions() const noexcept
  {
    return m_dimensions;
  }
  [[nodiscard]] inline constexpr WindowData* window_data() const noexcept
  {
    return m_window_data.get();
  }

private:
  std::string m_name{};
  bool m_running{};
  uvec2 m_dimensions{};
  Input m_input{};
  std::unique_ptr<WindowData> m_window_data{};
};

struct AudioDescription
{
  u32 sample_rate{48'000};
  u32 channels{2};
  u32 bit_count{16};
};

class Audio
{
public:
  struct AudioData
  {
    virtual ~AudioData() {}
  };

public:
  Audio(AudioDescription desc = {});
  Audio(const Audio&) = delete;
  Audio& operator=(const Audio&) = delete;
  Audio(Audio&& other);
  Audio& operator=(Audio&& other);
  ~Audio();

  u32 get_queued() const;
  void push(std::span<i16> buffer);

private:
  std::unique_ptr<AudioData> m_audio_data{};
};

}

#endif
