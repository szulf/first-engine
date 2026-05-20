#ifndef GAME_SOUND_H
#define GAME_SOUND_H

#include <thread>
#include <vector>
#include <filesystem>

#include "base/base.h"
#include "base/spsc_queue.h"
#include "os/os.h"

enum Sound_Handle
{
  SOUND_HANDLE_SINE,
  SOUND_HANDLE_SHOTGUN,
  SOUND_HANDLE_TEST_MUSIC,
  SOUND_HANDLE_COUNT,
};

// NOTE: assumes 48'000 sample rate, 2 channels and i16 encoding
struct Sound_Data
{
  std::vector<i16> samples{};
  u32 frames{};
};

std::expected<Sound_Data, std::string_view> sound_load_wav(const std::filesystem::path& path);

enum Sound_CmdType
{
  SOUND_CMD_PLAY_ONCE,
  SOUND_CMD_START_LOOP,
  SOUND_CMD_END_LOOP,
};

struct Sound_Cmd
{
  Sound_CmdType type{};
  Sound_Handle sound{};
  f32 volume{1.0f};
};

struct Sound_Source
{
  Sound_Handle handle;
  u32 frame_idx{};
  f32 volume{};
  bool loop{};
};

#define SOUND_BUFFER_FRAMES 512
#define SOUND_BUFFER_CHANNELS 2
#define SOUND_BUFFER_SIZE (SOUND_BUFFER_FRAMES * SOUND_BUFFER_CHANNELS)
#define SOUND_BUFFER_SIZE_BYTES (SOUND_BUFFER_SIZE * sizeof(i16))

struct Sound_System
{
  OS_Audio* audio{};
  std::jthread thread{};
  // NOTE: atomic value between 0 and 100
  u32 master_volume = 100;
  Sound_Data sound_data[SOUND_HANDLE_COUNT]{};
  SPSCQueue<Sound_Cmd> cmds{};
  std::vector<Sound_Source> active_sources{};
  i16 mix_buffer[SOUND_BUFFER_SIZE]{};
};

Sound_System sound_system_init(OS_Audio& audio);
void sound_system_start(Sound_System& system);
void sound_system_deinit(Sound_System& system);
void sound_play_once(Sound_System& system, Sound_Handle sound, f32 volume = 1.0f);
// NOTE: assumes only 1 looped sound source per sound handle
void sound_play_looped(Sound_System& system, Sound_Handle sound, f32 volume = 1.0f);
void sound_stop_looped(Sound_System& system, Sound_Handle sound);

#endif
