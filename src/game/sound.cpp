#include "sound.h"
#include "base/base.h"
#include "base/spsc_queue.h"
#include "os/os.h"

#include <fstream>
#include <cmath>

struct SoundDescription
{
  const char* file;
};

struct WAVContext
{
  usize curr_pos{};
  std::vector<u8> buffer{};
  SoundData out{};
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

SoundData load_wav(const std::filesystem::path& path)
{
  WAVContext ctx{};
  // NOTE: i absolutely hate this, but there is no easy way to read a binary file in the stl
  {
    std::ifstream file{path, std::ios::binary};
    if (file.fail())
    {
      throw std::runtime_error{std::format("[WAV] Failed to open file. (path: {}).", path.string())
      };
    }
    ctx.buffer.resize(std::filesystem::file_size(path));
    file.read((char*) ctx.buffer.data(), (i64) ctx.buffer.size());
  }

  if (!wav_expect(ctx, "RIFF"))
  {
    throw std::runtime_error{"[WAV] Invalid 'RIFF' master header."};
  }
  wav_read_u32(ctx); // file_size (?)
  if (!wav_expect(ctx, "WAVE"))
  {
    throw std::runtime_error{"[WAV] Invalid 'WAVE' master header."};
  }
  if (!wav_expect(ctx, "fmt "))
  {
    throw std::runtime_error{"[WAV] Invalid 'fmt ' header."};
  }
  wav_read_u32(ctx); // fmt_size
  u16 format_type = wav_read_u16(ctx);
  if (format_type != 1)
  {
    throw std::runtime_error{"[WAV] Invalid format_type. (Non PCM)."};
  }
  u16 channels = wav_read_u16(ctx);
  u32 sample_rate = wav_read_u32(ctx);
  if (sample_rate != 48'000)
  {
    throw std::runtime_error{"[WAV] Invalid sample rate."};
  }
  wav_read_u32(ctx); // idk
  wav_read_u16(ctx); // idk2
  u16 bits_per_sample = wav_read_u16(ctx);
  if (bits_per_sample != 16)
  {
    throw std::runtime_error{"[WAV] Invalid bits per sample count."};
  }
  if (!wav_expect(ctx, "data"))
  {
    throw std::runtime_error{"[WAV] Invalid 'data' header."};
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
  return ctx.out;
}

SoundSystem::SoundSystem(OS_Audio& audio) : m_audio{audio}
{
  m_cmds = spsc_queue_init<SoundCmd>(1024);
  {
    SoundData sound{};
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
    m_sound_data[SOUND_HANDLE_SINE] = std::move(sound);
  }

  m_sound_data[SOUND_HANDLE_SHOTGUN] = load_wav("assets/shotgun.wav");

  m_sound_data[SOUND_HANDLE_TEST_MUSIC] = load_wav("assets/music.wav");

  m_thread = std::jthread(
    [&](std::stop_token st)
    {
      sound_loop(st);
    }
  );
}

void SoundSystem::play_once(SoundHandle sound, f32 volume)
{
  spsc_queue_push(m_cmds, {.type = SOUND_CMD_PLAY_ONCE, .sound = sound, .volume = volume});
}

void SoundSystem::play_looped(SoundHandle sound, f32 volume)
{
  spsc_queue_push(m_cmds, {.type = SOUND_CMD_START_LOOP, .sound = sound, .volume = volume});
}

void SoundSystem::stop_looped(SoundHandle sound)
{
  spsc_queue_push(m_cmds, {.type = SOUND_CMD_END_LOOP, .sound = sound});
}

void SoundSystem::sound_loop(std::stop_token st)
{
  while (!st.stop_requested())
  {
    auto queued = os_audio_get_queued(m_audio);

    // TODO: definitely has one buffer of delay(around 10ms), keeping for now like mixing
    if (queued <= BYTES_PER_BUFFER)
    {
      SoundCmd cmd{};
      while (spsc_queue_consume_one(m_cmds, cmd))
      {
        switch (cmd.type)
        {
          case SOUND_CMD_PLAY_ONCE:
          {
            m_active_sources.push_back({
              .handle = cmd.sound,
              .volume = cmd.volume,
              .loop = false,
            });
          }
          break;
          case SOUND_CMD_START_LOOP:
          {
            m_active_sources.push_back({
              .handle = cmd.sound,
              .volume = cmd.volume,
              .loop = true,
            });
          }
          break;
          case SOUND_CMD_END_LOOP:
          {
            auto it = std::ranges::find_if(
              m_active_sources,
              [&cmd](const auto& v)
              {
                return v.handle == cmd.sound;
              }
            );
            if (it != m_active_sources.end())
            {
              m_active_sources.erase(it);
            }
          }
          break;
        }
      }

      auto master = master_volume.load();
      std::ranges::fill(mix_buffer, 0);
      for (auto it = m_active_sources.begin(); it != m_active_sources.end();)
      {
        SoundSource& v = *it;
        auto& data = m_sound_data[v.handle];
        for (u32 f = 0; f < FRAMES; ++f)
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
          mix_buffer[f * 2 + 0] = (i16) std::clamp(
            (mix_buffer[f * 2 + 0] + (data.samples[sample_index + 0] * v.volume * master)),
            (f32) std::numeric_limits<i16>::min(),
            (f32) std::numeric_limits<i16>::max()
          );
          mix_buffer[f * 2 + 1] = (i16) std::clamp(
            (mix_buffer[f * 2 + 1] + (data.samples[sample_index + 1] * v.volume * master)),
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
            it = m_active_sources.erase(it);
          }
        }
        else
        {
          ++it;
        }
      }

      os_audio_push(m_audio, mix_buffer);
    }
    else
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}
