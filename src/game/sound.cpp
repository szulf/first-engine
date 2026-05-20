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
  ctx.out.frames = data_size / (sizeof(i16) * channels);
  return {ctx.out};
}

void sound_loop(Sound_System& system, std::stop_token st)
{
  while (!st.stop_requested())
  {
    auto queued = os_audio_get_queued(*system.audio);

    // TODO: definitely has one buffer of delay(around 10ms), keeping for now like mixing
    if (queued <= SOUND_BUFFER_SIZE_BYTES)
    {
      Sound_Cmd cmd{};
      while (spsc_queue_consume_one(system.cmds, cmd))
      {
        switch (cmd.type)
        {
          case SOUND_CMD_PLAY_ONCE:
          {
            system.active_sources.push_back({
              .handle = cmd.sound,
              .volume = cmd.volume,
              .loop = false,
            });
          }
          break;
          case SOUND_CMD_START_LOOP:
          {
            system.active_sources.push_back({
              .handle = cmd.sound,
              .volume = cmd.volume,
              .loop = true,
            });
          }
          break;
          case SOUND_CMD_END_LOOP:
          {
            auto it = std::ranges::find_if(
              system.active_sources,
              [&cmd](const auto& v)
              {
                return v.handle == cmd.sound;
              }
            );
            if (it != system.active_sources.end())
            {
              system.active_sources.erase(it);
            }
          }
          break;
        }
      }

      auto master = atomic_load_32(&system.master_volume, ATOMIC_MEMORY_ORDER_RELAXED);
      ASSERT(master <= 100, "Invalid master volume value");
      std::ranges::fill(system.mix_buffer, 0);
      for (auto it = system.active_sources.begin(); it != system.active_sources.end();)
      {
        Sound_Source& v = *it;
        auto& data = system.sound_data[v.handle];
        for (u32 f = 0; f < SOUND_BUFFER_FRAMES; ++f)
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
          system.mix_buffer[f * 2 + 0] = (i16) std::clamp(
            (system.mix_buffer[f * 2 + 0] +
             (data.samples[sample_index + 0] * v.volume * ((f32) master / 100.0f))),
            (f32) std::numeric_limits<i16>::min(),
            (f32) std::numeric_limits<i16>::max()
          );
          system.mix_buffer[f * 2 + 1] = (i16) std::clamp(
            (system.mix_buffer[f * 2 + 1] +
             (data.samples[sample_index + 1] * v.volume * ((f32) master / 100.0f))),
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
            ++it;
          }
          else
          {
            it = system.active_sources.erase(it);
          }
        }
        else
        {
          ++it;
        }
      }

      os_audio_push(*system.audio, system.mix_buffer);
    }
    else
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

Sound_System sound_system_init(OS_Audio& audio)
{
  Sound_System system = {};
  system.audio = &audio;
  system.cmds = spsc_queue_init<Sound_Cmd>(1024);

  {
    Sound_Data sound{};
    sound.frames = (u32) (48'000 * 0.3f);
    sound.samples.resize(sound.frames * 2);
    static constexpr f32 phase_inc = 2.0f * std::numbers::pi_v<f32> * 440.0f / 48'000.0f;
    f32 phase{std::numbers::pi_v<f32> * 0.5f};
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
    system.sound_data[SOUND_HANDLE_SINE] = std::move(sound);
  }
  {
    auto sound = sound_load_wav("assets/shotgun.wav");
    if (sound)
    {
      system.sound_data[SOUND_HANDLE_SHOTGUN] = *sound;
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
      system.sound_data[SOUND_HANDLE_TEST_MUSIC] = *sound;
    }
    else
    {
      REPORT_ERROR(sound.error());
    }
  }

  return system;
}

void sound_system_start(Sound_System& system)
{
  system.thread = std::jthread(
    [&](std::stop_token st)
    {
      sound_loop(system, st);
    }
  );
}

void sound_system_deinit(Sound_System& system)
{
  system.thread.request_stop();
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
