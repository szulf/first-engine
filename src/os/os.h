#ifndef OS_OS_H
#define OS_OS_H

#include <expected>
#include <string>
#include <string_view>

#include "base/base.h"
#include "base/math.h"
#include "base/errors.h"

bool os_init();
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
  OS_KEY_0,
  OS_KEY_1,
  OS_KEY_2,
  OS_KEY_3,
  OS_KEY_4,
  OS_KEY_5,
  OS_KEY_6,
  OS_KEY_7,
  OS_KEY_8,
  OS_KEY_9,
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
  OS_KEY_TAB,
  OS_KEY_ESCAPE,
  OS_KEY_COUNT,
};

std::expected<OS_Key, Error> os_string_to_key(std::string_view str);

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

struct OS_WindowPlatformData;
struct OS_Window
{
  std::string name{};
  bool running{};
  vec2 dimensions{};
  OS_Input input{};
  OS_WindowPlatformData* platform_data{};
};

std::expected<OS_Window, Error> os_window_open(std::string_view name, const vec2& dimensions);
void os_window_close(OS_Window& window);
void os_window_update(OS_Window& window);
void os_window_swap_buffers(OS_Window& window);
void os_window_center_mouse_pointer(OS_Window& window);

static constexpr u32 OS_AUDIO_SAMPLE_RATE = 48000;
static constexpr u32 OS_AUDIO_CHANNELS = 2;
static constexpr u32 OS_AUDIO_SAMPLE_SIZE_BITS = 16;
static constexpr u32 OS_AUDIO_SAMPLE_SIZE_BYTES = OS_AUDIO_SAMPLE_SIZE_BITS / 8;

using OS_AudioCallback = void (*)(i16* buffer, usize bytes_to_fill, void* user_data);
struct OS_AudioPlatformData;
struct OS_Audio
{
  OS_AudioPlatformData* platform_data{};
};

std::expected<OS_Audio, Error> os_audio_init();
void os_audio_deinit(OS_Audio& audio);
void os_audio_set_callback(OS_Audio& audio, OS_AudioCallback callback, void* user_data);

#endif
