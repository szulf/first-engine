#ifndef GAME_SOUND_H
#define GAME_SOUND_H

#include <vector>
#include <filesystem>

#include "base/base.h"
#include "base/errors.h"
#include "base/spsc_queue.h"
#include "os/os.h"

enum Sound_Handle
{
  SOUND_SINE,
  SOUND_SHOTGUN,
  SOUND_TEST_MUSIC,
  SOUND_HANDLE_COUNT,
};

// NOTE: assumes 48'000 sample rate, 2 channels and i16 encoding
struct Sound_Data
{
  std::vector<i16> samples{};
  u32 frames{};
};

std::expected<Sound_Data, Error> sound_load_wav(const std::filesystem::path& path);

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

static constexpr usize SOUND_MAX_ACTIVE_SOURCES = 32;

// NOTE: not responsible for calling os_audio_deinit()
struct Sound_System
{
  OS_Audio* audio{};
  // NOTE: atomic value between 0 and 100
  u32 master_volume = 100;
  Sound_Data sound_data[SOUND_HANDLE_COUNT]{};
  SPSCQueue<Sound_Cmd> cmds{};
  std::array<Sound_Source, SOUND_MAX_ACTIVE_SOURCES> active_sources{};
  usize active_sources_count{};
};

void sound_system_init(Sound_System& system, OS_Audio& audio);
void sound_play_once(Sound_System& system, Sound_Handle sound, f32 volume = 1.0f);
// NOTE: assumes only 1 looped sound source per sound handle
void sound_play_looped(Sound_System& system, Sound_Handle sound, f32 volume = 1.0f);
void sound_stop_looped(Sound_System& system, Sound_Handle sound);

#endif
