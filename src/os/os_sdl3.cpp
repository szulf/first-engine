#include "os.h"

#include "base/base.h"
#include "sdl3/include/SDL3/SDL.h"

#include "gl_functions.h"

namespace os
{

void init()
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
}

void shutdown()
{
  SDL_Quit();
}

std::expected<std::string_view, std::string_view> key_to_string(Key key)
{
  switch (key)
  {
    case KEY_A:
      return {"A"};
    case KEY_B:
      return {"B"};
    case KEY_C:
      return {"C"};
    case KEY_D:
      return {"D"};
    case KEY_E:
      return {"E"};
    case KEY_F:
      return {"F"};
    case KEY_G:
      return {"G"};
    case KEY_H:
      return {"H"};
    case KEY_I:
      return {"I"};
    case KEY_J:
      return {"J"};
    case KEY_K:
      return {"K"};
    case KEY_L:
      return {"L"};
    case KEY_M:
      return {"M"};
    case KEY_N:
      return {"N"};
    case KEY_O:
      return {"O"};
    case KEY_P:
      return {"P"};
    case KEY_Q:
      return {"Q"};
    case KEY_R:
      return {"R"};
    case KEY_S:
      return {"S"};
    case KEY_T:
      return {"T"};
    case KEY_U:
      return {"U"};
    case KEY_V:
      return {"V"};
    case KEY_W:
      return {"W"};
    case KEY_X:
      return {"X"};
    case KEY_Y:
      return {"Y"};
    case KEY_Z:
      return {"Z"};
    case KEY_F1:
      return {"F1"};
    case KEY_F2:
      return {"F2"};
    case KEY_F3:
      return {"F3"};
    case KEY_F4:
      return {"F4"};
    case KEY_F5:
      return {"F5"};
    case KEY_F6:
      return {"F6"};
    case KEY_F7:
      return {"F7"};
    case KEY_F8:
      return {"F8"};
    case KEY_F9:
      return {"F9"};
    case KEY_F10:
      return {"F10"};
    case KEY_F11:
      return {"F11"};
    case KEY_F12:
      return {"F12"};
    case KEY_SPACE:
      return {"SPACE"};
    case KEY_LSHIFT:
      return {"LSHIFT"};
    default:
    case KEY_COUNT:
      return std::unexpected{"Invalid key."};
  }
}

std::expected<Key, std::string_view> string_to_key(std::string_view str)
{
  if (str == "A")
  {
    return {KEY_A};
  }
  else if (str == "B")
  {
    return {KEY_B};
  }
  else if (str == "C")
  {
    return {KEY_C};
  }
  else if (str == "D")
  {
    return {KEY_D};
  }
  else if (str == "E")
  {
    return {KEY_E};
  }
  else if (str == "F")
  {
    return {KEY_F};
  }
  else if (str == "G")
  {
    return {KEY_G};
  }
  else if (str == "H")
  {
    return {KEY_H};
  }
  else if (str == "I")
  {
    return {KEY_I};
  }
  else if (str == "J")
  {
    return {KEY_J};
  }
  else if (str == "K")
  {
    return {KEY_K};
  }
  else if (str == "L")
  {
    return {KEY_L};
  }
  else if (str == "M")
  {
    return {KEY_M};
  }
  else if (str == "N")
  {
    return {KEY_N};
  }
  else if (str == "O")
  {
    return {KEY_O};
  }
  else if (str == "P")
  {
    return {KEY_P};
  }
  else if (str == "Q")
  {
    return {KEY_Q};
  }
  else if (str == "R")
  {
    return {KEY_R};
  }
  else if (str == "S")
  {
    return {KEY_S};
  }
  else if (str == "T")
  {
    return {KEY_T};
  }
  else if (str == "U")
  {
    return {KEY_U};
  }
  else if (str == "V")
  {
    return {KEY_V};
  }
  else if (str == "W")
  {
    return {KEY_W};
  }
  else if (str == "X")
  {
    return {KEY_X};
  }
  else if (str == "Y")
  {
    return {KEY_Y};
  }
  else if (str == "Z")
  {
    return {KEY_Z};
  }
  else if (str == "F1")
  {
    return {KEY_F1};
  }
  else if (str == "F2")
  {
    return {KEY_F2};
  }
  else if (str == "F3")
  {
    return {KEY_F3};
  }
  else if (str == "F4")
  {
    return {KEY_F4};
  }
  else if (str == "F5")
  {
    return {KEY_F5};
  }
  else if (str == "F6")
  {
    return {KEY_F6};
  }
  else if (str == "F7")
  {
    return {KEY_F7};
  }
  else if (str == "F8")
  {
    return {KEY_F8};
  }
  else if (str == "F9")
  {
    return {KEY_F9};
  }
  else if (str == "F10")
  {
    return {KEY_F10};
  }
  else if (str == "F11")
  {
    return {KEY_F11};
  }
  else if (str == "F12")
  {
    return {KEY_F12};
  }
  else if (str == "SPACE")
  {
    return {KEY_SPACE};
  }
  else if (str == "LSHIFT")
  {
    return {KEY_LSHIFT};
  }

  return std::unexpected{"Invalid key string"};
}

void Input::clear()
{
  for (auto& key : keys)
  {
    key.transition_count = 0;
  }
  lmb.transition_count = 0;
  rmb.transition_count = 0;
  mouse_scroll = 0;
}

struct SDL3WindowData : public Window::WindowData
{
  SDL_Window* window;
  SDL_GLContext gl_context;
};

Window::Window(std::string_view name, uvec2 dimensions)
  : m_name{name}, m_dimensions{dimensions}, m_window_data{std::make_unique<SDL3WindowData>()}
{
  auto& data = static_cast<SDL3WindowData&>(*m_window_data);

  data.window = SDL_CreateWindow(
    m_name.data(),
    (i32) m_dimensions.x,
    (i32) m_dimensions.y,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
  );
  ASSERT(data.window, "Failed to open window. (msg: {})", SDL_GetError());
  m_running = true;

#ifdef GAME_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  data.gl_context = SDL_GL_CreateContext(data.window);
  ASSERT(data.gl_context, "Failed to create OpenGL context. (msg: {})", SDL_GetError());
  setup_gl_functions();
  // SDL_GL_SetSwapInterval(0);

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, nullptr);
#endif
}

Window::Window(Window&& other)
{
  m_name = std::move(other.m_name);
  m_running = other.m_running;
  other.m_running = false;
  m_dimensions = other.m_dimensions;
  m_input = other.m_input;
  m_window_data = std::move(other.m_window_data);
}

Window& Window::operator=(Window&& other)
{
  if (this == &other)
  {
    return *this;
  }
  m_name = std::move(other.m_name);
  m_running = other.m_running;
  other.m_running = false;
  m_dimensions = other.m_dimensions;
  m_input = other.m_input;
  ASSERT(m_window_data, "Invalid window state.");
  auto& data = static_cast<SDL3WindowData&>(*m_window_data);
  if (data.gl_context)
  {
    SDL_GL_DestroyContext(data.gl_context);
  }
  if (data.window)
  {
    SDL_DestroyWindow(data.window);
  }
  m_window_data = std::move(other.m_window_data);
  return *this;
}

Window::~Window()
{
  ASSERT(m_window_data, "Invalid window state.");
  auto& data = static_cast<SDL3WindowData&>(*m_window_data);
  if (data.gl_context)
  {
    SDL_GL_DestroyContext(data.gl_context);
  }
  if (data.window)
  {
    SDL_DestroyWindow(data.window);
  }
}

static std::expected<SDL_Keycode, std::string_view> sdlk_from_key(Key key)
{
  switch (key)
  {
    case KEY_A:
      return {SDLK_A};
    case KEY_B:
      return {SDLK_B};
    case KEY_C:
      return {SDLK_C};
    case KEY_D:
      return {SDLK_D};
    case KEY_E:
      return {SDLK_E};
    case KEY_F:
      return {SDLK_F};
    case KEY_G:
      return {SDLK_G};
    case KEY_H:
      return {SDLK_H};
    case KEY_I:
      return {SDLK_I};
    case KEY_J:
      return {SDLK_J};
    case KEY_K:
      return {SDLK_K};
    case KEY_L:
      return {SDLK_L};
    case KEY_M:
      return {SDLK_M};
    case KEY_N:
      return {SDLK_N};
    case KEY_O:
      return {SDLK_O};
    case KEY_P:
      return {SDLK_P};
    case KEY_Q:
      return {SDLK_Q};
    case KEY_R:
      return {SDLK_R};
    case KEY_S:
      return {SDLK_S};
    case KEY_T:
      return {SDLK_T};
    case KEY_U:
      return {SDLK_U};
    case KEY_V:
      return {SDLK_V};
    case KEY_W:
      return {SDLK_W};
    case KEY_X:
      return {SDLK_X};
    case KEY_Y:
      return {SDLK_Y};
    case KEY_Z:
      return {SDLK_Z};
    case KEY_F1:
      return {SDLK_F1};
    case KEY_F2:
      return {SDLK_F2};
    case KEY_F3:
      return {SDLK_F3};
    case KEY_F4:
      return {SDLK_F4};
    case KEY_F5:
      return {SDLK_F5};
    case KEY_F6:
      return {SDLK_F6};
    case KEY_F7:
      return {SDLK_F7};
    case KEY_F8:
      return {SDLK_F8};
    case KEY_F9:
      return {SDLK_F9};
    case KEY_F10:
      return {SDLK_F10};
    case KEY_F11:
      return {SDLK_F11};
    case KEY_F12:
      return {SDLK_F12};
    case KEY_SPACE:
      return {SDLK_SPACE};
    case KEY_LSHIFT:
      return {SDLK_LSHIFT};
    case KEY_COUNT:
    default:
      return std::unexpected{"Invalid key provided"};
  }
}

static std::expected<Key, std::string_view> key_from_sdlk(SDL_Keycode sdlk)
{
  switch (sdlk)
  {
    case SDLK_A:
      return {KEY_A};
    case SDLK_B:
      return {KEY_B};
    case SDLK_C:
      return {KEY_C};
    case SDLK_D:
      return {KEY_D};
    case SDLK_E:
      return {KEY_E};
    case SDLK_F:
      return {KEY_F};
    case SDLK_G:
      return {KEY_G};
    case SDLK_H:
      return {KEY_H};
    case SDLK_I:
      return {KEY_I};
    case SDLK_J:
      return {KEY_J};
    case SDLK_K:
      return {KEY_K};
    case SDLK_L:
      return {KEY_L};
    case SDLK_M:
      return {KEY_M};
    case SDLK_N:
      return {KEY_N};
    case SDLK_O:
      return {KEY_O};
    case SDLK_P:
      return {KEY_P};
    case SDLK_Q:
      return {KEY_Q};
    case SDLK_R:
      return {KEY_R};
    case SDLK_S:
      return {KEY_S};
    case SDLK_T:
      return {KEY_T};
    case SDLK_U:
      return {KEY_U};
    case SDLK_V:
      return {KEY_V};
    case SDLK_W:
      return {KEY_W};
    case SDLK_X:
      return {KEY_X};
    case SDLK_Y:
      return {KEY_Y};
    case SDLK_Z:
      return {KEY_Z};
    case SDLK_F1:
      return {KEY_F1};
    case SDLK_F2:
      return {KEY_F2};
    case SDLK_F3:
      return {KEY_F3};
    case SDLK_F4:
      return {KEY_F4};
    case SDLK_F5:
      return {KEY_F5};
    case SDLK_F6:
      return {KEY_F6};
    case SDLK_F7:
      return {KEY_F7};
    case SDLK_F8:
      return {KEY_F8};
    case SDLK_F9:
      return {KEY_F9};
    case SDLK_F10:
      return {KEY_F10};
    case SDLK_F11:
      return {KEY_F11};
    case SDLK_F12:
      return {KEY_F12};
    case SDLK_SPACE:
      return {KEY_SPACE};
    case SDLK_LSHIFT:
      return {KEY_LSHIFT};
  }
  return std::unexpected{"Invalid key provided"};
}

void Window::update()
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    switch (e.type)
    {
      case SDL_EVENT_QUIT:
      {
        m_running = false;
      }
      break;
      case SDL_EVENT_WINDOW_RESIZED:
      {
        m_dimensions.x = (u32) e.window.data1;
        m_dimensions.y = (u32) e.window.data2;
      }
      break;
      case SDL_EVENT_KEY_UP:
      case SDL_EVENT_KEY_DOWN:
      {
        auto key = key_from_sdlk(e.key.key);
        if (key)
        {
          ++m_input.key(*key).transition_count;
        }
      }
      break;
      case SDL_EVENT_MOUSE_MOTION:
      {
        m_input.mouse_pos = {e.motion.x, e.motion.y};
        m_input.mouse_delta = {e.motion.xrel, e.motion.yrel};
      }
      break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      {
        if (e.button.button == 1)
        {
          m_input.lmb.down = e.button.down;
          ++m_input.lmb.transition_count;
        }
        else if (e.button.button == 3)
        {
          m_input.rmb.down = e.button.down;
          ++m_input.rmb.transition_count;
        }
      }
      break;
      case SDL_EVENT_MOUSE_WHEEL:
      {
        m_input.mouse_scroll += e.wheel.integer_y;
      }
      break;
    }
  }

  const bool* key_states = SDL_GetKeyboardState(nullptr);
  for (usize i = 0; i < KEY_COUNT; ++i)
  {
    auto sdlk = sdlk_from_key((Key) i);
    if (sdlk)
    {
      m_input.keys[i].down = key_states[SDL_GetScancodeFromKey(*sdlk, nullptr)];
    }
  }
}

void Window::swap_buffers()
{
  ASSERT(m_window_data, "Invalid window state.");
  auto& data = static_cast<SDL3WindowData&>(*m_window_data);
  SDL_GL_SwapWindow(data.window);
}

void Window::hide_mouse_pointer()
{
  ASSERT(m_window_data, "Invalid window state.");
  auto& data = static_cast<SDL3WindowData&>(*m_window_data);
  SDL_HideCursor();
  SDL_WarpMouseInWindow(data.window, (f32) m_dimensions.x / 2.0f, (f32) m_dimensions.y / 2.0f);
}

void Window::show_mouse_pointer()
{
  SDL_ShowCursor();
}

struct SDL3AudioData : public Audio::AudioData
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

Audio::Audio(AudioDescription desc) : m_audio_data{std::make_unique<SDL3AudioData>()}
{
  auto audio_format = bit_count_to_sdl_audio_format(desc.bit_count);
  if (!audio_format)
  {
    throw std::runtime_error{"Invalid audio description bit count provided."};
  }
  SDL_AudioSpec spec{
    .format = *audio_format,
    .channels = static_cast<i32>(desc.channels),
    .freq = static_cast<i32>(desc.sample_rate),
  };
  auto& data = static_cast<SDL3AudioData&>(*m_audio_data);
  data.stream =
    SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
  SDL_ResumeAudioStreamDevice(data.stream);
}

Audio::Audio(Audio&& other)
{
  m_audio_data = std::move(other.m_audio_data);
}

Audio& Audio::operator=(Audio&& other)
{
  if (this == &other)
  {
    return *this;
  }
  if (m_audio_data)
  {
    auto& data = static_cast<SDL3AudioData&>(*m_audio_data);
    SDL_DestroyAudioStream(data.stream);
  }
  m_audio_data = std::move(other.m_audio_data);
  return *this;
}

Audio::~Audio()
{
  auto& data = static_cast<SDL3AudioData&>(*m_audio_data);
  SDL_DestroyAudioStream(data.stream);
}

u32 Audio::get_queued() const
{
  auto& data = static_cast<SDL3AudioData&>(*m_audio_data);
  return (u32) SDL_GetAudioStreamQueued(data.stream);
}

void Audio::push(std::span<i16> buffer)
{
  auto& data = static_cast<SDL3AudioData&>(*m_audio_data);
  SDL_PutAudioStreamData(data.stream, buffer.data(), (i32) (buffer.size() * sizeof(i16)));
}

}
