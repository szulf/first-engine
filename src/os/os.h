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

f32 os_get_time();
u64 os_get_time_ns();

void os_show_mouse_pointer();
void os_hide_mouse_pointer();

#define OS_KEYS                                                                                    \
  X(OS_KEY_A, "A")                                                                                 \
  X(OS_KEY_B, "B")                                                                                 \
  X(OS_KEY_C, "C")                                                                                 \
  X(OS_KEY_D, "D")                                                                                 \
  X(OS_KEY_E, "E")                                                                                 \
  X(OS_KEY_F, "F")                                                                                 \
  X(OS_KEY_G, "G")                                                                                 \
  X(OS_KEY_H, "H")                                                                                 \
  X(OS_KEY_I, "I")                                                                                 \
  X(OS_KEY_J, "J")                                                                                 \
  X(OS_KEY_K, "K")                                                                                 \
  X(OS_KEY_L, "L")                                                                                 \
  X(OS_KEY_M, "M")                                                                                 \
  X(OS_KEY_N, "N")                                                                                 \
  X(OS_KEY_O, "O")                                                                                 \
  X(OS_KEY_P, "P")                                                                                 \
  X(OS_KEY_Q, "Q")                                                                                 \
  X(OS_KEY_R, "R")                                                                                 \
  X(OS_KEY_S, "S")                                                                                 \
  X(OS_KEY_T, "T")                                                                                 \
  X(OS_KEY_U, "U")                                                                                 \
  X(OS_KEY_V, "V")                                                                                 \
  X(OS_KEY_W, "W")                                                                                 \
  X(OS_KEY_X, "X")                                                                                 \
  X(OS_KEY_Y, "Y")                                                                                 \
  X(OS_KEY_Z, "Z")                                                                                 \
  X(OS_KEY_0, "0")                                                                                 \
  X(OS_KEY_1, "1")                                                                                 \
  X(OS_KEY_2, "2")                                                                                 \
  X(OS_KEY_3, "3")                                                                                 \
  X(OS_KEY_4, "4")                                                                                 \
  X(OS_KEY_5, "5")                                                                                 \
  X(OS_KEY_6, "6")                                                                                 \
  X(OS_KEY_7, "7")                                                                                 \
  X(OS_KEY_8, "8")                                                                                 \
  X(OS_KEY_9, "9")                                                                                 \
  X(OS_KEY_F1, "F1")                                                                               \
  X(OS_KEY_F2, "F2")                                                                               \
  X(OS_KEY_F3, "F3")                                                                               \
  X(OS_KEY_F4, "F4")                                                                               \
  X(OS_KEY_F5, "F5")                                                                               \
  X(OS_KEY_F6, "F6")                                                                               \
  X(OS_KEY_F7, "F7")                                                                               \
  X(OS_KEY_F8, "F8")                                                                               \
  X(OS_KEY_F9, "F9")                                                                               \
  X(OS_KEY_F10, "F10")                                                                             \
  X(OS_KEY_F11, "F11")                                                                             \
  X(OS_KEY_F12, "F12")                                                                             \
  X(OS_KEY_SPACE, "SPACE")                                                                         \
  X(OS_KEY_LSHIFT, "LSHIFT")                                                                       \
  X(OS_KEY_TAB, "TAB")                                                                             \
  X(OS_KEY_ESCAPE, "ESCAPE")

#define X(key, str) key,
enum OS_Key
{
  OS_KEYS OS_KEY_COUNT,
};
#undef X

#define X(key, str) map[key] = str;
static constexpr std::array<std::string_view, OS_KEY_COUNT> OS_KEY_TO_STRING = []()
{
  std::array<std::string_view, OS_KEY_COUNT> map{};
  OS_KEYS
  return map;
}();
#undef X

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
