#include "base/base.h"

#include <chrono>

#include "os/os.h"
#include "game/game.h"

static constexpr i32 TPS{60};
static constexpr std::chrono::milliseconds DT{
  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds{1}) / TPS
};
static constexpr f32 DT_F32{(f32) DT.count() / (f32) std::milli::den};

i32 main()
{
  os_init();
  defer(os_shutdown());

  auto window = os_window_open("game", {1280, 720});
  ASSERT(window, "Failed to create window");
  defer(os_window_close(*window));

  auto audio = os_audio_init();
  ASSERT(audio, "Failed to init audio");
  defer(os_audio_uninit(*audio));

  Game game{*window, *audio};

  auto current_time = std::chrono::high_resolution_clock::now();
  std::chrono::nanoseconds accumulator{};
  while (window->running)
  {
    auto new_time = std::chrono::high_resolution_clock::now();
    auto frame_time = new_time - current_time;
    current_time = new_time;
    accumulator += frame_time;

    os_window_update(*window);

    while (accumulator >= DT)
    {
      game.update_tick(DT_F32);
      os_input_clear(window->input);
      accumulator -= DT;
    }

    game.update_frame(((f32) accumulator.count() / (f32) std::nano::den) / DT_F32);

    game.render();

    // auto end_time = std::chrono::high_resolution_clock::now();
    // std::println(
    //   "Frame time: {}",
    //   ((f32) (end_time - current_time).count() / (f32) std::nano::den) * (f32) std::milli::den
    // );

    os_window_swap_buffers(*window);
  }

  return 0;
}
