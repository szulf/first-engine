#ifndef OS_H
#define OS_H

#include <expected>
#include <memory>
#include <string>
#include <string_view>

#include "base/base.h"
#include "base/math.h"

namespace os
{

void init();
void shutdown();

enum Key
{
  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,
  KEY_SPACE,
  KEY_LSHIFT,
  KEY_COUNT,
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

  KeyState keys[KEY_COUNT];
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
