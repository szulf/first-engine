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
  os::init();
  os::Window window{"game", {1280, 720}};
  os::Audio audio{};
  Game game{window, audio};

  auto current_time = std::chrono::high_resolution_clock::now();
  std::chrono::nanoseconds accumulator{};
  while (window.running())
  {
    auto new_time = std::chrono::high_resolution_clock::now();
    auto frame_time = new_time - current_time;
    current_time = new_time;
    accumulator += frame_time;

    window.update();

    while (accumulator >= DT)
    {
      game.update_tick(DT_F32);
      window.input().clear();
      accumulator -= DT;
    }

    game.update_frame(((f32) accumulator.count() / (f32) std::nano::den) / DT_F32);

    game.render();

    // auto end_time = std::chrono::high_resolution_clock::now();
    // std::println(
    //   "Frame time: {}",
    //   ((f32) (end_time - current_time).count() / (f32) std::nano::den) * (f32) std::milli::den
    // );

    window.swap_buffers();
  }

  return 0;
}
