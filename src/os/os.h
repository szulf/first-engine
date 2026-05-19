#ifndef OS_H
#define OS_H

#include <expected>
#include <string>
#include <string_view>

#include "base/base.h"
#include "base/math.h"

void os_init();
void os_shutdown();

void os_show_mouse_pointer();
void os_hide_mouse_pointer();

enum OS_Key
{
  OS_KEY_A,
  OS_KEY_B,
  OS_KEY_C,
  OS_KEY_D,
  OS_KEY_E,
  OS_KEY_F,
  OS_KEY_G,
  OS_KEY_H,
  OS_KEY_I,
  OS_KEY_J,
  OS_KEY_K,
  OS_KEY_L,
  OS_KEY_M,
  OS_KEY_N,
  OS_KEY_O,
  OS_KEY_P,
  OS_KEY_Q,
  OS_KEY_R,
  OS_KEY_S,
  OS_KEY_T,
  OS_KEY_U,
  OS_KEY_V,
  OS_KEY_W,
  OS_KEY_X,
  OS_KEY_Y,
  OS_KEY_Z,
  OS_KEY_F1,
  OS_KEY_F2,
  OS_KEY_F3,
  OS_KEY_F4,
  OS_KEY_F5,
  OS_KEY_F6,
  OS_KEY_F7,
  OS_KEY_F8,
  OS_KEY_F9,
  OS_KEY_F10,
  OS_KEY_F11,
  OS_KEY_F12,
  OS_KEY_SPACE,
  OS_KEY_LSHIFT,
  OS_KEY_COUNT,
};

std::expected<OS_Key, std::string_view> os_string_to_key(std::string_view str);

struct OS_KeyState
{
  u32 transition_count;
  bool down;
};

inline bool os_key_just_pressed(const OS_KeyState& key)
{
  return key.down && key.transition_count != 0;
}

struct OS_Input
{
  OS_KeyState keys[OS_KEY_COUNT];
  OS_KeyState lmb;
  OS_KeyState rmb;
  i32 mouse_scroll;
  vec2 mouse_pos;
  vec2 mouse_delta;
};

void os_input_clear(OS_Input& input);

struct OS_Window
{
  std::string name{};
  bool running{};
  uvec2 dimensions{};
  OS_Input input{};
  struct PlatformData;
  PlatformData* platform_data{};
};

std::expected<OS_Window, std::string_view>
os_window_open(std::string_view name, const uvec2& dimensions);
void os_window_close(OS_Window& window);
void os_window_update(OS_Window& window);
void os_window_swap_buffers(OS_Window& window);
void os_window_center_mouse_pointer(OS_Window& window);

struct OS_Audio
{
  struct PlatformData;
  PlatformData* platform_data{};
};

std::expected<OS_Audio, std::string_view>
os_audio_init(u32 sample_rate = 48000, u32 channels = 2, u32 bit_count = 16);
void os_audio_deinit(OS_Audio& audio);
u32 os_audio_get_queued(const OS_Audio& audio);
void os_audio_push(OS_Audio& audio, std::span<i16> buffer);

#endif
