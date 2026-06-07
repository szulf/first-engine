#include "sound.h"

#include <fstream>
#include <cmath>

#include "base/base.h"
#include "base/spsc_queue.h"
#include "base/errors.h"
#include "os/os.h"

struct WAVContext
{
  usize curr_pos{};
  std::vector<u8> buffer{};
  Sound_Data out{};
};

inline static u8 wav_read_u8(WAVContext& ctx)
{
  return ctx.buffer[ctx.curr_pos++];
}

inline static u16 wav_read_u16(WAVContext& ctx)
{
  return (wav_read_u8(ctx)) | (u16) (wav_read_u8(ctx) << 8);
}

inline static u32 wav_read_u32(WAVContext& ctx)
{
  return (wav_read_u16(ctx)) | (u32) (wav_read_u16(ctx) << 16);
}

inline static bool wav_expect(WAVContext& ctx, std::string_view str)
{
  for (char c : str)
  {
    if (ctx.buffer[ctx.curr_pos++] != c)
    {
      return false;
    }
  }
  return true;
}

enum WaveFormat
{
  WAVE_FORMAT_PCM = 0x0001,
  WAVE_FORMAT_IEEE_FLOAT = 0x0003,
  WAVE_FORMAT_ALAW = 0x0006,
  WAVE_FORMAT_MULAW = 0x0007,
  WAVE_FORMAT_EXTENSIBLE = 0xFFFE,
};

std::expected<Sound_Data, std::string_view> sound_load_wav(const std::filesystem::path& path)
{
  WAVContext ctx{};
  // NOTE: i absolutely hate this, but there is no easy way to read a binary file in the stl
  {
    std::ifstream file{path, std::ios::binary};
    if (file.fail())
    {
      return std::unexpected{"Failed to open wav file"};
    }
    ctx.buffer.resize(std::filesystem::file_size(path));
    file.read((char*) ctx.buffer.data(), (i64) ctx.buffer.size());
  }

  if (!wav_expect(ctx, "RIFF"))
  {
    return std::unexpected{"Invalid 'RIFF' master header"};
  }
  wav_read_u32(ctx); // file_size (?)
  if (!wav_expect(ctx, "WAVE"))
  {
    return std::unexpected{"Invalid 'WAVE' master header"};
  }
  if (!wav_expect(ctx, "fmt "))
  {
    return std::unexpected{"Invalid 'fmt ' header"};
  }
  wav_read_u32(ctx); // fmt_size
  u16 format_type = wav_read_u16(ctx);
  if (format_type != 1)
  {
    return std::unexpected{"Invalid format_type - Non PCM"};
  }
  u16 channels = wav_read_u16(ctx);
  u32 sample_rate = wav_read_u32(ctx);
  if (sample_rate != 48'000)
  {
    return std::unexpected{"Invalid sample rate"};
  }
  wav_read_u32(ctx); // idk
  wav_read_u16(ctx); // idk2
  u16 bits_per_sample = wav_read_u16(ctx);
  if (bits_per_sample != 16)
  {
    return std::unexpected{"Invalid bits per sample count"};
  }
  if (!wav_expect(ctx, "data"))
  {
    return std::unexpected{"Invalid 'data' header"};
  }
  u32 data_size = wav_read_u32(ctx);
  ctx.out.samples.reserve(data_size);
  const usize end = data_size + ctx.curr_pos;
  while (ctx.curr_pos < end)
  {
    i16 sample = (i16) wav_read_u16(ctx);
    ctx.out.samples.push_back(sample);
  }
  ctx.out.frames = data_size / (OS_AUDIO_SAMPLE_SIZE_BYTES * channels);
  return {ctx.out};
}

static void sound_callback(i16* buffer, usize bytes_to_fill, void* user_data)
{
  ASSERT(user_data, "Invalid sound system pointer passed into callback");
  auto& system = *(Sound_System*) user_data;
  Sound_Cmd cmd{};
  while (spsc_queue_consume_one(system.cmds, cmd))
  {
    // TODO: not decided yet whether this should be an ASSERT() or a REPORT_ERROR()
    switch (cmd.type)
    {
      case SOUND_CMD_PLAY_ONCE:
      {
        ASSERT(
          system.active_sources_count < SOUND_MAX_ACTIVE_SOURCES,
          "Exceeded max amount of active sound sources"
        );
        system.active_sources[system.active_sources_count] = {
          .handle = cmd.sound,
          .volume = cmd.volume,
          .loop = false,
        };
        ++system.active_sources_count;
      }
      break;
      case SOUND_CMD_START_LOOP:
      {
        ASSERT(
          system.active_sources_count < SOUND_MAX_ACTIVE_SOURCES,
          "Exceeded max amount of active sound sources"
        );
        system.active_sources[system.active_sources_count] = {
          .handle = cmd.sound,
          .volume = cmd.volume,
          .loop = true,
        };
        ++system.active_sources_count;
      }
      break;
      case SOUND_CMD_END_LOOP:
      {
        for (usize i = 0; i < system.active_sources_count; ++i)
        {
          if (system.active_sources[i].handle == cmd.sound)
          {
            system.active_sources[i] = system.active_sources[system.active_sources_count - 1];
            --system.active_sources_count;
            break;
          }
        }
      }
      break;
    }
  }

  auto master = atomic_load_32(&system.master_volume, ATOMIC_MEMORY_ORDER_RELAXED);
  ASSERT(master <= 100, "Invalid master volume value");

  usize frame_count = bytes_to_fill / (OS_AUDIO_SAMPLE_SIZE_BYTES * OS_AUDIO_CHANNELS);
  for (usize idx = 0; idx < system.active_sources_count;)
  {
    Sound_Source& v = system.active_sources[idx];
    auto& data = system.sound_data[v.handle];
    for (u32 f = 0; f < frame_count; ++f)
    {
      if (v.frame_idx >= data.frames)
      {
        break;
      }

      // TODO: this is slighly better than last time, but still pretty bad
      // im gonna keep this for now(need to get some things done in the engine),
      // but i need to later probably watch how to do proper sound mixing
      // probably will watch the "handmade hero" series,
      // it seems like an overall good source of information
      u32 sample_index = v.frame_idx * 2;
      buffer[f * 2 + 0] = (i16) std::clamp(
        (buffer[f * 2 + 0] + (data.samples[sample_index + 0] * v.volume * ((f32) master / 100.0f))),
        (f32) std::numeric_limits<i16>::min(),
        (f32) std::numeric_limits<i16>::max()
      );
      buffer[f * 2 + 1] = (i16) std::clamp(
        (buffer[f * 2 + 1] + (data.samples[sample_index + 1] * v.volume * ((f32) master / 100.0f))),
        (f32) std::numeric_limits<i16>::min(),
        (f32) std::numeric_limits<i16>::max()
      );

      ++v.frame_idx;
    }
    if (v.frame_idx >= data.frames)
    {
      if (v.loop)
      {
        v.frame_idx = 0;
        ++idx;
      }
      else
      {
        v = system.active_sources[system.active_sources_count - 1];
        --system.active_sources_count;
      }
    }
    else
    {
      ++idx;
    }
  }
}

void sound_system_init(Sound_System& system, OS_Audio& audio)
{
  system = {};
  system.audio = &audio;
  system.cmds = spsc_queue_init<Sound_Cmd>(1024);

  os_audio_set_callback(audio, sound_callback, (void*) &system);

  {
    auto& sound = system.sound_data[SOUND_SINE];
    sound.frames = (u32) (48'000 * 0.3f);
    sound.samples.resize(sound.frames * 2);
    static constexpr f32 phase_inc = 2.0f * std::numbers::pi_v<f32> * 440.0f / 48'000.0f;
    f32 phase = std::numbers::pi_v<f32> * 0.5f;
    for (u32 frame = 0; frame < sound.frames; ++frame)
    {
      f32 s = std::sin(phase) * 0.5f;
      phase += phase_inc;
      if (phase >= 2.0f * std::numbers::pi_v<f32>)
      {
        phase -= 2.0f * std::numbers::pi_v<f32>;
      }
      i16 sample = (i16) (s * std::numeric_limits<i16>::max());
      sound.samples[frame * 2 + 0] = sample;
      sound.samples[frame * 2 + 1] = sample;
    }
  }
  {
    auto sound = sound_load_wav("assets/shotgun.wav");
    if (sound)
    {
      system.sound_data[SOUND_SHOTGUN] = *sound;
    }
    else
    {
      REPORT_ERROR(sound.error());
    }
  }
  {
    auto sound = sound_load_wav("assets/music.wav");
    if (sound)
    {
      system.sound_data[SOUND_TEST_MUSIC] = *sound;
    }
    else
    {
      REPORT_ERROR(sound.error());
    }
  }
}

void sound_play_once(Sound_System& system, Sound_Handle sound, f32 volume)
{
  spsc_queue_push(system.cmds, {.type = SOUND_CMD_PLAY_ONCE, .sound = sound, .volume = volume});
}

void sound_play_looped(Sound_System& system, Sound_Handle sound, f32 volume)
{
  spsc_queue_push(system.cmds, {.type = SOUND_CMD_START_LOOP, .sound = sound, .volume = volume});
}

void sound_stop_looped(Sound_System& system, Sound_Handle sound)
{
  spsc_queue_push(system.cmds, {.type = SOUND_CMD_END_LOOP, .sound = sound});
}
