#include "os.h"

#include <expected>
#include "sdl3/include/SDL3/SDL.h"

#include "base/base.h"
#include "gl_functions.h"

bool os_init()
{
  return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
}

void os_shutdown()
{
  SDL_Quit();
}

f32 os_get_time()
{
  return (f32) SDL_GetTicksNS() / 1000000000.0f;
}

u64 os_get_time_ns()
{
  return SDL_GetTicksNS();
}

void os_hide_mouse_pointer()
{
  SDL_HideCursor();
}

void os_show_mouse_pointer()
{
  SDL_ShowCursor();
}

std::expected<OS_Key, Error> os_string_to_key(std::string_view str)
{
  for (usize i = 0; i < OS_KEY_COUNT; ++i)
  {
    if (str == OS_KEY_TO_STRING[(OS_Key) i])
    {
      return {(OS_Key) i};
    }
  }
  return std::unexpected{ERROR("Invalid key string: {}", str)};
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

struct OS_WindowPlatformData
{
  SDL_Window* window{};
  SDL_GLContext gl_context{};
};

std::expected<OS_Window, Error> os_window_open(std::string_view name, const vec2& dimensions)
{
#ifdef MODE_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  OS_Window window = {
    .name = std::string{name},
    .dimensions = dimensions,
    .platform_data = (OS_WindowPlatformData*) malloc(sizeof(OS_WindowPlatformData)),
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
    free(window.platform_data);
    return std::unexpected{ERROR("Failed to open window")};
  }
  window.running = true;

  window.platform_data->gl_context = SDL_GL_CreateContext(window.platform_data->window);
  if (!window.platform_data->gl_context)
  {
    SDL_DestroyWindow(window.platform_data->window);
    free(window.platform_data);
    return std::unexpected{ERROR("Failed to create OpenGL context")};
  }
  setup_gl_functions();
  // SDL_GL_SetSwapInterval(0);

#ifdef MODE_DEBUG
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

static constexpr std::array<SDL_Keycode, OS_KEY_COUNT> OS_KEY_TO_SDLK = []()
{
  std::array<SDL_Keycode, OS_KEY_COUNT> map{};
  map[OS_KEY_A] = SDLK_A;
  map[OS_KEY_B] = SDLK_B;
  map[OS_KEY_C] = SDLK_C;
  map[OS_KEY_D] = SDLK_D;
  map[OS_KEY_E] = SDLK_E;
  map[OS_KEY_F] = SDLK_F;
  map[OS_KEY_G] = SDLK_G;
  map[OS_KEY_H] = SDLK_H;
  map[OS_KEY_I] = SDLK_I;
  map[OS_KEY_J] = SDLK_J;
  map[OS_KEY_K] = SDLK_K;
  map[OS_KEY_L] = SDLK_L;
  map[OS_KEY_M] = SDLK_M;
  map[OS_KEY_N] = SDLK_N;
  map[OS_KEY_O] = SDLK_O;
  map[OS_KEY_P] = SDLK_P;
  map[OS_KEY_Q] = SDLK_Q;
  map[OS_KEY_R] = SDLK_R;
  map[OS_KEY_S] = SDLK_S;
  map[OS_KEY_T] = SDLK_T;
  map[OS_KEY_U] = SDLK_U;
  map[OS_KEY_V] = SDLK_V;
  map[OS_KEY_W] = SDLK_W;
  map[OS_KEY_X] = SDLK_X;
  map[OS_KEY_Y] = SDLK_Y;
  map[OS_KEY_Z] = SDLK_Z;
  map[OS_KEY_0] = SDLK_0;
  map[OS_KEY_1] = SDLK_1;
  map[OS_KEY_2] = SDLK_2;
  map[OS_KEY_3] = SDLK_3;
  map[OS_KEY_4] = SDLK_4;
  map[OS_KEY_5] = SDLK_5;
  map[OS_KEY_6] = SDLK_6;
  map[OS_KEY_7] = SDLK_7;
  map[OS_KEY_8] = SDLK_8;
  map[OS_KEY_9] = SDLK_9;
  map[OS_KEY_F1] = SDLK_F1;
  map[OS_KEY_F2] = SDLK_F2;
  map[OS_KEY_F3] = SDLK_F3;
  map[OS_KEY_F4] = SDLK_F4;
  map[OS_KEY_F5] = SDLK_F5;
  map[OS_KEY_F6] = SDLK_F6;
  map[OS_KEY_F7] = SDLK_F7;
  map[OS_KEY_F8] = SDLK_F8;
  map[OS_KEY_F9] = SDLK_F9;
  map[OS_KEY_F10] = SDLK_F10;
  map[OS_KEY_F11] = SDLK_F11;
  map[OS_KEY_F12] = SDLK_F12;
  map[OS_KEY_SPACE] = SDLK_SPACE;
  map[OS_KEY_LSHIFT] = SDLK_LSHIFT;
  map[OS_KEY_TAB] = SDLK_TAB;
  map[OS_KEY_ESCAPE] = SDLK_ESCAPE;
  return map;
}();

static std::expected<OS_Key, Error> sdlk_to_os_key(SDL_Keycode sdlk)
{
  for (usize i = 0; i < OS_KEY_COUNT; ++i)
  {
    if (sdlk == OS_KEY_TO_SDLK[(OS_Key) i])
    {
      return {(OS_Key) i};
    }
  }
  return std::unexpected{ERROR("Invalid sdlk: {}", sdlk)};
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
        auto key = sdlk_to_os_key(e.key.key);
        if (key)
        {
          ++window.input.keys[*key].transition_count;
        }
      }
      break;
      case SDL_EVENT_MOUSE_MOTION:
      {
        window.input.mouse_pos = {e.motion.x, e.motion.y};
      }
      break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      {
        if (e.button.button == SDL_BUTTON_LEFT)
        {
          window.input.lmb.down = e.button.down;
          ++window.input.lmb.transition_count;
        }
        else if (e.button.button == SDL_BUTTON_RIGHT)
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
    window.input.keys[i].down =
      key_states[SDL_GetScancodeFromKey(OS_KEY_TO_SDLK[(OS_Key) i], nullptr)];
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
  window.input.mouse_pos = {(f32) window.dimensions.x / 2.0f, (f32) window.dimensions.y / 2.0f};
}

static constexpr usize OS_SDL_MIX_BUFFER_SIZE = 16384;
struct OS_SDL_CallbackData
{
  OS_AudioCallback user_callback{};
  void* user_data{};
  i16* mix_buffer{};
};

struct OS_AudioPlatformData
{
  SDL_AudioStream* stream{};
  OS_SDL_CallbackData* callback_data{};
};

static std::expected<SDL_AudioFormat, Error> bit_count_to_sdl_audio_format(u32 bit_count)
{
  switch (bit_count)
  {
    case 16:
      return SDL_AUDIO_S16;
    default:
      return std::unexpected{ERROR("Invalid bit count: {}", bit_count)};
  }
}

std::expected<OS_Audio, Error> os_audio_init()
{
  OS_Audio audio = {
    .platform_data = (OS_AudioPlatformData*) malloc(sizeof(OS_AudioPlatformData)),
    // NOTE: allocates memory for 1 second of audio, SURELY it will never ask for this much at once
  };
  ASSERT(audio.platform_data, "Failed to alloc audio platform data");

  audio.platform_data->callback_data = (OS_SDL_CallbackData*) malloc(sizeof(OS_SDL_CallbackData));
  ASSERT(audio.platform_data->callback_data, "Failed to alloc audio callback data");
  audio.platform_data->callback_data->mix_buffer = (i16*) malloc(OS_SDL_MIX_BUFFER_SIZE);
  ASSERT(audio.platform_data->callback_data->mix_buffer, "Failed to alloc audio mixing buffer");

  SDL_AudioSpec spec = {
    .channels = (i32) OS_AUDIO_CHANNELS,
    .freq = (i32) OS_AUDIO_SAMPLE_RATE,
  };
  if (auto bit_count = bit_count_to_sdl_audio_format(OS_AUDIO_SAMPLE_SIZE_BITS))
  {
    spec.format = *bit_count;
  }
  else
  {
    free(audio.platform_data->callback_data->mix_buffer);
    free(audio.platform_data->callback_data);
    free(audio.platform_data);
    return std::unexpected{FORWARD(bit_count.error())};
  }

  audio.platform_data->stream =
    SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
  if (!audio.platform_data->stream)
  {
    free(audio.platform_data->callback_data->mix_buffer);
    free(audio.platform_data->callback_data);
    free(audio.platform_data);
    return std::unexpected{ERROR("Failed to initialize audio:\n{}", SDL_GetError())};
  }
  return {audio};
}

void os_audio_deinit(OS_Audio& audio)
{
  SDL_DestroyAudioStream(audio.platform_data->stream);
  free(audio.platform_data->callback_data->mix_buffer);
  free(audio.platform_data->callback_data);
  free(audio.platform_data);
}

static void
os_sdl_callback(void* user_data, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
  // TODO: not really sure which one to use additional_amount or total_amount,
  // they seem to be always equal on my machine
  UNUSED(total_amount);
  auto data = (OS_SDL_CallbackData*) user_data;
  ASSERT(
    OS_SDL_MIX_BUFFER_SIZE > (usize) additional_amount,
    "Audio mixing buffer is not big enough"
  );
  std::memset(data->mix_buffer, 0, (usize) additional_amount);
  data->user_callback(data->mix_buffer, (usize) additional_amount, data->user_data);
  SDL_PutAudioStreamData(stream, data->mix_buffer, additional_amount);
}

void os_audio_set_callback(OS_Audio& audio, OS_AudioCallback callback, void* user_data)
{
  audio.platform_data->callback_data->user_callback = callback;
  audio.platform_data->callback_data->user_data = user_data;
  bool ok = SDL_SetAudioStreamGetCallback(
    audio.platform_data->stream,
    os_sdl_callback,
    audio.platform_data->callback_data
  );
  ASSERT(ok, "Failed to set audio callback:\n{}", SDL_GetError());
  ok = SDL_ResumeAudioStreamDevice(audio.platform_data->stream);
  ASSERT(ok, "Failed to start audio:\n{}", SDL_GetError());
}
