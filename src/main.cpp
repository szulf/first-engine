#include <chrono>

#include "base/base.h"
#include "base/errors.h"
#include "os/os.h"
#include "game/game.h"

static constexpr i32 TPS = 60;
static constexpr f32 DT = 1.0f / (f32) TPS;

i32 main()
{
  bool ok = os_init();
  if (!ok)
  {
    FATAL("Failed to init os layer");
  }
  defer(os_shutdown());

  auto window = os_window_open("game", {1280, 720});
  if (!window)
  {
    FATAL("Failed to create window:\n{}", error_to_string(window.error()));
  }
  defer(os_window_close(*window));

  auto audio = os_audio_init();
  if (!audio)
  {
    FATAL("Failed to init audio:\n{}", error_to_string(audio.error()));
  }
  defer(os_audio_deinit(*audio));

  GameData game{};
  game_init(game, *window, *audio);
  defer(game_deinit(game));

  auto current_time = os_get_time();
  f32 accumulator{};
  while (window->running)
  {
    auto new_time = os_get_time();
    auto frame_time = new_time - current_time;
    // TODO: i dont know if i really want this
    frame_time = std::min(frame_time, 0.25f);
    current_time = new_time;
    accumulator += frame_time;

    os_window_update(*window);

    while (accumulator >= DT)
    {
      game_update_tick(game, DT);
      os_input_clear(window->input);
      accumulator -= DT;
    }

    f32 t = accumulator / DT;
    game_update_frame(game, t);
    game_render(game, t);

    // auto end_time = std::chrono::high_resolution_clock::now();
    // std::println(
    //   "Frame time: {}",
    //   ((f32) (end_time - current_time).count() / (f32) std::nano::den) * (f32) std::milli::den
    // );

    os_window_swap_buffers(*window);
  }

  return 0;
}
