#pragma once

#include <thread>
#include <vector>
#include <filesystem>

#include "base/base.h"
#include "base/spsc_queue.h"

#include "os/os.h"

enum SoundHandle
{
  SOUND_HANDLE_SINE,
  SOUND_HANDLE_SHOTGUN,
  SOUND_HANDLE_TEST_MUSIC,
  SOUND_HANDLE_COUNT,
};

// NOTE: assumes 48'000 sample rate, 2 channels and i16 encoding
struct SoundData
{
  std::vector<i16> samples{};
  u32 frames{};
};

SoundData load_wav(const std::filesystem::path& path);

enum SoundCmdType
{
  SOUND_CMD_PLAY_ONCE,
  SOUND_CMD_START_LOOP,
  SOUND_CMD_END_LOOP,
};

struct SoundCmd
{
  SoundCmdType type{};
  SoundHandle sound{};
  f32 volume{1.0f};
};

struct SoundSource
{
  SoundHandle handle;
  u32 frame_idx{};
  f32 volume{};
  bool loop{};
};

class SoundSystem
{
public:
  SoundSystem(OS_Audio& audio);
  ~SoundSystem()
  {
    m_thread.request_stop();
    m_thread.join();
    spsc_queue_uninit(m_cmds);
  }

  void play_once(SoundHandle sound, f32 volume = 1.0f);
  // NOTE: assumes only 1 looped sound source per sound handle
  void play_looped(SoundHandle sound, f32 volume = 1.0f);
  void stop_looped(SoundHandle sound);

private:
  void sound_loop(std::stop_token st);

public:
  std::atomic<f32> master_volume{1.0f};

private:
  std::jthread m_thread{};

  OS_Audio& m_audio;

  SPSCQueue<SoundCmd> m_cmds{};
  SoundData m_sound_data[SOUND_HANDLE_COUNT]{};
  std::vector<SoundSource> m_active_sources{};

  static constexpr u32 FRAMES = 512;
  static constexpr u32 CHANNELS = 2;
  static constexpr usize BYTES_PER_BUFFER = FRAMES * CHANNELS * sizeof(i16);
  std::array<i16, FRAMES * CHANNELS> mix_buffer{};
};
