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
    case Key::A:
      return {"A"};
    case Key::B:
      return {"B"};
    case Key::C:
      return {"C"};
    case Key::D:
      return {"D"};
    case Key::E:
      return {"E"};
    case Key::F:
      return {"F"};
    case Key::G:
      return {"G"};
    case Key::H:
      return {"H"};
    case Key::I:
      return {"I"};
    case Key::J:
      return {"J"};
    case Key::K:
      return {"K"};
    case Key::L:
      return {"L"};
    case Key::M:
      return {"M"};
    case Key::N:
      return {"N"};
    case Key::O:
      return {"O"};
    case Key::P:
      return {"P"};
    case Key::Q:
      return {"Q"};
    case Key::R:
      return {"R"};
    case Key::S:
      return {"S"};
    case Key::T:
      return {"T"};
    case Key::U:
      return {"U"};
    case Key::V:
      return {"V"};
    case Key::W:
      return {"W"};
    case Key::X:
      return {"X"};
    case Key::Y:
      return {"Y"};
    case Key::Z:
      return {"Z"};
    case Key::F1:
      return {"F1"};
    case Key::F2:
      return {"F2"};
    case Key::F3:
      return {"F3"};
    case Key::F4:
      return {"F4"};
    case Key::F5:
      return {"F5"};
    case Key::F6:
      return {"F6"};
    case Key::F7:
      return {"F7"};
    case Key::F8:
      return {"F8"};
    case Key::F9:
      return {"F9"};
    case Key::F10:
      return {"F10"};
    case Key::F11:
      return {"F11"};
    case Key::F12:
      return {"F12"};
    case Key::SPACE:
      return {"SPACE"};
    case Key::LSHIFT:
      return {"LSHIFT"};
    default:
    case Key::COUNT:
      return std::unexpected{"Invalid key."};
  }
}

std::expected<Key, std::string_view> string_to_key(std::string_view str)
{
  if (str == "A")
  {
    return {Key::A};
  }
  else if (str == "B")
  {
    return {Key::B};
  }
  else if (str == "C")
  {
    return {Key::C};
  }
  else if (str == "D")
  {
    return {Key::D};
  }
  else if (str == "E")
  {
    return {Key::E};
  }
  else if (str == "F")
  {
    return {Key::F};
  }
  else if (str == "G")
  {
    return {Key::G};
  }
  else if (str == "H")
  {
    return {Key::H};
  }
  else if (str == "I")
  {
    return {Key::I};
  }
  else if (str == "J")
  {
    return {Key::J};
  }
  else if (str == "K")
  {
    return {Key::K};
  }
  else if (str == "L")
  {
    return {Key::L};
  }
  else if (str == "M")
  {
    return {Key::M};
  }
  else if (str == "N")
  {
    return {Key::N};
  }
  else if (str == "O")
  {
    return {Key::O};
  }
  else if (str == "P")
  {
    return {Key::P};
  }
  else if (str == "Q")
  {
    return {Key::Q};
  }
  else if (str == "R")
  {
    return {Key::R};
  }
  else if (str == "S")
  {
    return {Key::S};
  }
  else if (str == "T")
  {
    return {Key::T};
  }
  else if (str == "U")
  {
    return {Key::U};
  }
  else if (str == "V")
  {
    return {Key::V};
  }
  else if (str == "W")
  {
    return {Key::W};
  }
  else if (str == "X")
  {
    return {Key::X};
  }
  else if (str == "Y")
  {
    return {Key::Y};
  }
  else if (str == "Z")
  {
    return {Key::Z};
  }
  else if (str == "F1")
  {
    return {Key::F1};
  }
  else if (str == "F2")
  {
    return {Key::F2};
  }
  else if (str == "F3")
  {
    return {Key::F3};
  }
  else if (str == "F4")
  {
    return {Key::F4};
  }
  else if (str == "F5")
  {
    return {Key::F5};
  }
  else if (str == "F6")
  {
    return {Key::F6};
  }
  else if (str == "F7")
  {
    return {Key::F7};
  }
  else if (str == "F8")
  {
    return {Key::F8};
  }
  else if (str == "F9")
  {
    return {Key::F9};
  }
  else if (str == "F10")
  {
    return {Key::F10};
  }
  else if (str == "F11")
  {
    return {Key::F11};
  }
  else if (str == "F12")
  {
    return {Key::F12};
  }
  else if (str == "SPACE")
  {
    return {Key::SPACE};
  }
  else if (str == "LSHIFT")
  {
    return {Key::LSHIFT};
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
    case Key::A:
      return {SDLK_A};
    case Key::B:
      return {SDLK_B};
    case Key::C:
      return {SDLK_C};
    case Key::D:
      return {SDLK_D};
    case Key::E:
      return {SDLK_E};
    case Key::F:
      return {SDLK_F};
    case Key::G:
      return {SDLK_G};
    case Key::H:
      return {SDLK_H};
    case Key::I:
      return {SDLK_I};
    case Key::J:
      return {SDLK_J};
    case Key::K:
      return {SDLK_K};
    case Key::L:
      return {SDLK_L};
    case Key::M:
      return {SDLK_M};
    case Key::N:
      return {SDLK_N};
    case Key::O:
      return {SDLK_O};
    case Key::P:
      return {SDLK_P};
    case Key::Q:
      return {SDLK_Q};
    case Key::R:
      return {SDLK_R};
    case Key::S:
      return {SDLK_S};
    case Key::T:
      return {SDLK_T};
    case Key::U:
      return {SDLK_U};
    case Key::V:
      return {SDLK_V};
    case Key::W:
      return {SDLK_W};
    case Key::X:
      return {SDLK_X};
    case Key::Y:
      return {SDLK_Y};
    case Key::Z:
      return {SDLK_Z};
    case Key::F1:
      return {SDLK_F1};
    case Key::F2:
      return {SDLK_F2};
    case Key::F3:
      return {SDLK_F3};
    case Key::F4:
      return {SDLK_F4};
    case Key::F5:
      return {SDLK_F5};
    case Key::F6:
      return {SDLK_F6};
    case Key::F7:
      return {SDLK_F7};
    case Key::F8:
      return {SDLK_F8};
    case Key::F9:
      return {SDLK_F9};
    case Key::F10:
      return {SDLK_F10};
    case Key::F11:
      return {SDLK_F11};
    case Key::F12:
      return {SDLK_F12};
    case Key::SPACE:
      return {SDLK_SPACE};
    case Key::LSHIFT:
      return {SDLK_LSHIFT};
    case Key::COUNT:
    default:
      return std::unexpected{"Invalid key provided"};
  }
}

static std::expected<Key, std::string_view> key_from_sdlk(SDL_Keycode sdlk)
{
  switch (sdlk)
  {
    case SDLK_A:
      return {Key::A};
    case SDLK_B:
      return {Key::B};
    case SDLK_C:
      return {Key::C};
    case SDLK_D:
      return {Key::D};
    case SDLK_E:
      return {Key::E};
    case SDLK_F:
      return {Key::F};
    case SDLK_G:
      return {Key::G};
    case SDLK_H:
      return {Key::H};
    case SDLK_I:
      return {Key::I};
    case SDLK_J:
      return {Key::J};
    case SDLK_K:
      return {Key::K};
    case SDLK_L:
      return {Key::L};
    case SDLK_M:
      return {Key::M};
    case SDLK_N:
      return {Key::N};
    case SDLK_O:
      return {Key::O};
    case SDLK_P:
      return {Key::P};
    case SDLK_Q:
      return {Key::Q};
    case SDLK_R:
      return {Key::R};
    case SDLK_S:
      return {Key::S};
    case SDLK_T:
      return {Key::T};
    case SDLK_U:
      return {Key::U};
    case SDLK_V:
      return {Key::V};
    case SDLK_W:
      return {Key::W};
    case SDLK_X:
      return {Key::X};
    case SDLK_Y:
      return {Key::Y};
    case SDLK_Z:
      return {Key::Z};
    case SDLK_F1:
      return {Key::F1};
    case SDLK_F2:
      return {Key::F2};
    case SDLK_F3:
      return {Key::F3};
    case SDLK_F4:
      return {Key::F4};
    case SDLK_F5:
      return {Key::F5};
    case SDLK_F6:
      return {Key::F6};
    case SDLK_F7:
      return {Key::F7};
    case SDLK_F8:
      return {Key::F8};
    case SDLK_F9:
      return {Key::F9};
    case SDLK_F10:
      return {Key::F10};
    case SDLK_F11:
      return {Key::F11};
    case SDLK_F12:
      return {Key::F12};
    case SDLK_SPACE:
      return {Key::SPACE};
    case SDLK_LSHIFT:
      return {Key::LSHIFT};
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
      }
      break;
    }
  }

  const bool* key_states = SDL_GetKeyboardState(nullptr);
  for (usize i = 0; i < m_input.keys.size(); ++i)
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
