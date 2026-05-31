#include "os.h"

#include <expected>
#include "sdl3/include/SDL3/SDL.h"

#include "base/base.h"
#include "gl_functions.h"

void os_init()
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
}

void os_shutdown()
{
  SDL_Quit();
}

void os_hide_mouse_pointer()
{
  SDL_HideCursor();
}

void os_show_mouse_pointer()
{
  SDL_ShowCursor();
}

std::expected<OS_Key, std::string_view> os_string_to_key(std::string_view str)
{
  if (str == "A")
  {
    return {OS_KEY_A};
  }
  else if (str == "B")
  {
    return {OS_KEY_B};
  }
  else if (str == "C")
  {
    return {OS_KEY_C};
  }
  else if (str == "D")
  {
    return {OS_KEY_D};
  }
  else if (str == "E")
  {
    return {OS_KEY_E};
  }
  else if (str == "F")
  {
    return {OS_KEY_F};
  }
  else if (str == "G")
  {
    return {OS_KEY_G};
  }
  else if (str == "H")
  {
    return {OS_KEY_H};
  }
  else if (str == "I")
  {
    return {OS_KEY_I};
  }
  else if (str == "J")
  {
    return {OS_KEY_J};
  }
  else if (str == "K")
  {
    return {OS_KEY_K};
  }
  else if (str == "L")
  {
    return {OS_KEY_L};
  }
  else if (str == "M")
  {
    return {OS_KEY_M};
  }
  else if (str == "N")
  {
    return {OS_KEY_N};
  }
  else if (str == "O")
  {
    return {OS_KEY_O};
  }
  else if (str == "P")
  {
    return {OS_KEY_P};
  }
  else if (str == "Q")
  {
    return {OS_KEY_Q};
  }
  else if (str == "R")
  {
    return {OS_KEY_R};
  }
  else if (str == "S")
  {
    return {OS_KEY_S};
  }
  else if (str == "T")
  {
    return {OS_KEY_T};
  }
  else if (str == "U")
  {
    return {OS_KEY_U};
  }
  else if (str == "V")
  {
    return {OS_KEY_V};
  }
  else if (str == "W")
  {
    return {OS_KEY_W};
  }
  else if (str == "X")
  {
    return {OS_KEY_X};
  }
  else if (str == "Y")
  {
    return {OS_KEY_Y};
  }
  else if (str == "Z")
  {
    return {OS_KEY_Z};
  }
  else if (str == "F1")
  {
    return {OS_KEY_F1};
  }
  else if (str == "F2")
  {
    return {OS_KEY_F2};
  }
  else if (str == "F3")
  {
    return {OS_KEY_F3};
  }
  else if (str == "F4")
  {
    return {OS_KEY_F4};
  }
  else if (str == "F5")
  {
    return {OS_KEY_F5};
  }
  else if (str == "F6")
  {
    return {OS_KEY_F6};
  }
  else if (str == "F7")
  {
    return {OS_KEY_F7};
  }
  else if (str == "F8")
  {
    return {OS_KEY_F8};
  }
  else if (str == "F9")
  {
    return {OS_KEY_F9};
  }
  else if (str == "F10")
  {
    return {OS_KEY_F10};
  }
  else if (str == "F11")
  {
    return {OS_KEY_F11};
  }
  else if (str == "F12")
  {
    return {OS_KEY_F12};
  }
  else if (str == "SPACE")
  {
    return {OS_KEY_SPACE};
  }
  else if (str == "LSHIFT")
  {
    return {OS_KEY_LSHIFT};
  }
  else if (str == "TAB")
  {
    return {OS_KEY_TAB};
  }
  else if (str == "ESCAPE")
  {
    return {OS_KEY_ESCAPE};
  }

  return std::unexpected{"Invalid key string"};
}

void os_input_clear(OS_Input& input)
{
  for (usize i = 0; i < OS_KEY_COUNT; ++i)
  {
    input.keys[i].transition_count = 0;
  }
  input.lmb.transition_count = 0;
  input.rmb.transition_count = 0;
  input.mouse_scroll = 0;
}

struct OS_Window::PlatformData
{
  SDL_Window* window{};
  SDL_GLContext gl_context{};
};

std::expected<OS_Window, std::string_view>
os_window_open(std::string_view name, const vec2& dimensions)
{
  OS_Window window = {
    .name = std::string{name},
    .dimensions = dimensions,
    .platform_data = (OS_Window::PlatformData*) malloc(sizeof(OS_Window::PlatformData)),
  };
  ASSERT(window.platform_data, "Failed to alloc window platform data");
  window.platform_data->window = SDL_CreateWindow(
    window.name.c_str(),
    (i32) window.dimensions.x,
    (i32) window.dimensions.y,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
  );
  if (!window.platform_data->window)
  {
    return std::unexpected{"Failed to open window"};
  }
  window.running = true;

#ifdef GAME_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  window.platform_data->gl_context = SDL_GL_CreateContext(window.platform_data->window);
  if (!window.platform_data->gl_context)
  {
    return std::unexpected{"Failed to create OpenGL context"};
  }
  setup_gl_functions();
  // SDL_GL_SetSwapInterval(0);

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, nullptr);
#endif
  return {window};
}

void os_window_close(OS_Window& window)
{
  SDL_GL_DestroyContext(window.platform_data->gl_context);
  SDL_DestroyWindow(window.platform_data->window);
  free(window.platform_data);
}

static std::expected<SDL_Keycode, std::string_view> sdlk_from_key(OS_Key key)
{
  switch (key)
  {
    case OS_KEY_A:
      return {SDLK_A};
    case OS_KEY_B:
      return {SDLK_B};
    case OS_KEY_C:
      return {SDLK_C};
    case OS_KEY_D:
      return {SDLK_D};
    case OS_KEY_E:
      return {SDLK_E};
    case OS_KEY_F:
      return {SDLK_F};
    case OS_KEY_G:
      return {SDLK_G};
    case OS_KEY_H:
      return {SDLK_H};
    case OS_KEY_I:
      return {SDLK_I};
    case OS_KEY_J:
      return {SDLK_J};
    case OS_KEY_K:
      return {SDLK_K};
    case OS_KEY_L:
      return {SDLK_L};
    case OS_KEY_M:
      return {SDLK_M};
    case OS_KEY_N:
      return {SDLK_N};
    case OS_KEY_O:
      return {SDLK_O};
    case OS_KEY_P:
      return {SDLK_P};
    case OS_KEY_Q:
      return {SDLK_Q};
    case OS_KEY_R:
      return {SDLK_R};
    case OS_KEY_S:
      return {SDLK_S};
    case OS_KEY_T:
      return {SDLK_T};
    case OS_KEY_U:
      return {SDLK_U};
    case OS_KEY_V:
      return {SDLK_V};
    case OS_KEY_W:
      return {SDLK_W};
    case OS_KEY_X:
      return {SDLK_X};
    case OS_KEY_Y:
      return {SDLK_Y};
    case OS_KEY_Z:
      return {SDLK_Z};
    case OS_KEY_0:
      return {SDLK_0};
    case OS_KEY_1:
      return {SDLK_1};
    case OS_KEY_2:
      return {SDLK_2};
    case OS_KEY_3:
      return {SDLK_3};
    case OS_KEY_4:
      return {SDLK_4};
    case OS_KEY_5:
      return {SDLK_5};
    case OS_KEY_6:
      return {SDLK_6};
    case OS_KEY_7:
      return {SDLK_7};
    case OS_KEY_8:
      return {SDLK_8};
    case OS_KEY_9:
      return {SDLK_9};
    case OS_KEY_F1:
      return {SDLK_F1};
    case OS_KEY_F2:
      return {SDLK_F2};
    case OS_KEY_F3:
      return {SDLK_F3};
    case OS_KEY_F4:
      return {SDLK_F4};
    case OS_KEY_F5:
      return {SDLK_F5};
    case OS_KEY_F6:
      return {SDLK_F6};
    case OS_KEY_F7:
      return {SDLK_F7};
    case OS_KEY_F8:
      return {SDLK_F8};
    case OS_KEY_F9:
      return {SDLK_F9};
    case OS_KEY_F10:
      return {SDLK_F10};
    case OS_KEY_F11:
      return {SDLK_F11};
    case OS_KEY_F12:
      return {SDLK_F12};
    case OS_KEY_SPACE:
      return {SDLK_SPACE};
    case OS_KEY_LSHIFT:
      return {SDLK_LSHIFT};
    case OS_KEY_TAB:
      return {SDLK_TAB};
    case OS_KEY_ESCAPE:
      return {SDLK_ESCAPE};
    case OS_KEY_COUNT:
    default:
      return std::unexpected{"Invalid key provided"};
  }
}

static std::expected<OS_Key, std::string_view> key_from_sdlk(SDL_Keycode sdlk)
{
  switch (sdlk)
  {
    case SDLK_A:
      return {OS_KEY_A};
    case SDLK_B:
      return {OS_KEY_B};
    case SDLK_C:
      return {OS_KEY_C};
    case SDLK_D:
      return {OS_KEY_D};
    case SDLK_E:
      return {OS_KEY_E};
    case SDLK_F:
      return {OS_KEY_F};
    case SDLK_G:
      return {OS_KEY_G};
    case SDLK_H:
      return {OS_KEY_H};
    case SDLK_I:
      return {OS_KEY_I};
    case SDLK_J:
      return {OS_KEY_J};
    case SDLK_K:
      return {OS_KEY_K};
    case SDLK_L:
      return {OS_KEY_L};
    case SDLK_M:
      return {OS_KEY_M};
    case SDLK_N:
      return {OS_KEY_N};
    case SDLK_O:
      return {OS_KEY_O};
    case SDLK_P:
      return {OS_KEY_P};
    case SDLK_Q:
      return {OS_KEY_Q};
    case SDLK_R:
      return {OS_KEY_R};
    case SDLK_S:
      return {OS_KEY_S};
    case SDLK_T:
      return {OS_KEY_T};
    case SDLK_U:
      return {OS_KEY_U};
    case SDLK_V:
      return {OS_KEY_V};
    case SDLK_W:
      return {OS_KEY_W};
    case SDLK_X:
      return {OS_KEY_X};
    case SDLK_Y:
      return {OS_KEY_Y};
    case SDLK_Z:
      return {OS_KEY_Z};
    case SDLK_0:
      return {OS_KEY_0};
    case SDLK_1:
      return {OS_KEY_1};
    case SDLK_2:
      return {OS_KEY_2};
    case SDLK_3:
      return {OS_KEY_3};
    case SDLK_4:
      return {OS_KEY_4};
    case SDLK_5:
      return {OS_KEY_5};
    case SDLK_6:
      return {OS_KEY_6};
    case SDLK_7:
      return {OS_KEY_7};
    case SDLK_8:
      return {OS_KEY_8};
    case SDLK_9:
      return {OS_KEY_9};
    case SDLK_F1:
      return {OS_KEY_F1};
    case SDLK_F2:
      return {OS_KEY_F2};
    case SDLK_F3:
      return {OS_KEY_F3};
    case SDLK_F4:
      return {OS_KEY_F4};
    case SDLK_F5:
      return {OS_KEY_F5};
    case SDLK_F6:
      return {OS_KEY_F6};
    case SDLK_F7:
      return {OS_KEY_F7};
    case SDLK_F8:
      return {OS_KEY_F8};
    case SDLK_F9:
      return {OS_KEY_F9};
    case SDLK_F10:
      return {OS_KEY_F10};
    case SDLK_F11:
      return {OS_KEY_F11};
    case SDLK_F12:
      return {OS_KEY_F12};
    case SDLK_SPACE:
      return {OS_KEY_SPACE};
    case SDLK_LSHIFT:
      return {OS_KEY_LSHIFT};
    case SDLK_TAB:
      return {OS_KEY_TAB};
    case SDLK_ESCAPE:
      return {OS_KEY_ESCAPE};
  }
  return std::unexpected{"Invalid key provided"};
}

void os_window_update(OS_Window& window)
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    switch (e.type)
    {
      case SDL_EVENT_QUIT:
      {
        window.running = false;
      }
      break;
      case SDL_EVENT_WINDOW_RESIZED:
      {
        window.dimensions.x = (f32) e.window.data1;
        window.dimensions.y = (f32) e.window.data2;
      }
      break;
      case SDL_EVENT_KEY_UP:
      case SDL_EVENT_KEY_DOWN:
      {
        auto key = key_from_sdlk(e.key.key);
        if (key)
        {
          ++window.input.keys[*key].transition_count;
        }
      }
      break;
      case SDL_EVENT_MOUSE_MOTION:
      {
        window.input.mouse_pos = {e.motion.x, e.motion.y};
        window.input.mouse_delta = {e.motion.xrel, e.motion.yrel};
      }
      break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      {
        if (e.button.button == 1)
        {
          window.input.lmb.down = e.button.down;
          ++window.input.lmb.transition_count;
        }
        else if (e.button.button == 3)
        {
          window.input.rmb.down = e.button.down;
          ++window.input.rmb.transition_count;
        }
      }
      break;
      case SDL_EVENT_MOUSE_WHEEL:
      {
        window.input.mouse_scroll += e.wheel.integer_y;
      }
      break;
    }
  }

  const bool* key_states = SDL_GetKeyboardState(nullptr);
  for (usize i = 0; i < OS_KEY_COUNT; ++i)
  {
    auto sdlk = sdlk_from_key((OS_Key) i);
    if (sdlk)
    {
      window.input.keys[i].down = key_states[SDL_GetScancodeFromKey(*sdlk, nullptr)];
    }
  }
}

void os_window_swap_buffers(OS_Window& window)
{
  SDL_GL_SwapWindow(window.platform_data->window);
}

void os_window_center_mouse_pointer(OS_Window& window)
{
  SDL_WarpMouseInWindow(
    window.platform_data->window,
    (f32) window.dimensions.x / 2.0f,
    (f32) window.dimensions.y / 2.0f
  );
}

struct OS_Audio::PlatformData
{
  SDL_AudioStream* stream{};
};

static std::expected<SDL_AudioFormat, std::string_view> bit_count_to_sdl_audio_format(u32 bit_count)
{
  switch (bit_count)
  {
    case 16:
      return SDL_AUDIO_S16;
    default:
      return std::unexpected{"Invalid bit count provided."};
  }
}

std::expected<OS_Audio, std::string_view>
os_audio_init(u32 sample_rate, u32 channels, u32 bit_count)
{
  OS_Audio audio = {
    .platform_data = (OS_Audio::PlatformData*) malloc(sizeof(OS_Audio::PlatformData)),
  };
  ASSERT(audio.platform_data, "Failed to alloc audio platform data");
  auto audio_format = bit_count_to_sdl_audio_format(bit_count);
  if (!audio_format)
  {
    return std::unexpected{"Invalid audio description bit count provided"};
  }
  SDL_AudioSpec spec = {
    .format = *audio_format,
    .channels = (i32) channels,
    .freq = (i32) sample_rate,
  };
  audio.platform_data->stream =
    SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
  SDL_ResumeAudioStreamDevice(audio.platform_data->stream);
  return audio;
}

void os_audio_deinit(OS_Audio& audio)
{
  SDL_DestroyAudioStream(audio.platform_data->stream);
  free(audio.platform_data);
}

u32 os_audio_get_queued(const OS_Audio& audio)
{
  return (u32) SDL_GetAudioStreamQueued(audio.platform_data->stream);
}

void os_audio_push(OS_Audio& audio, std::span<i16> buffer)
{
  SDL_PutAudioStreamData(
    audio.platform_data->stream,
    buffer.data(),
    (i32) (buffer.size() * sizeof(i16))
  );
}
