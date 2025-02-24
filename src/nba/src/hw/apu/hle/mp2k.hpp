/*
 * Copyright (C) 2021 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <nba/integer.hpp>

namespace nba::core {

struct Bus;

struct MP2K {
  static constexpr u8 kMaxSoundChannels = 12;

  enum SoundChannelStatus : u8 {
    CHANNEL_START = 0x80,
    CHANNEL_STOP = 0x40,
    CHANNEL_LOOP = 0x10,
    CHANNEL_ECHO = 0x04,

    CHANNEL_ENV_MASK = 0x03,
    CHANNEL_ENV_ATTACK = 0x03,
    CHANNEL_ENV_DECAY = 0x02,
    CHANNEL_ENV_SUSTAIN = 0x01,
    CHANNEL_ENV_RELEASE = 0x00,
    
    CHANNEL_ON = CHANNEL_START | CHANNEL_STOP | CHANNEL_ECHO | CHANNEL_ENV_MASK 
  };

  struct SoundChannel {
    u8 status;
    u8 type;
    u8 volume_r;
    u8 volume_l;
    u8 envelope_attack;
    u8 envelope_decay;
    u8 envelope_sustain;
    u8 envelope_release;
    u8 unknown0;
    u8 envelope_volume;
    u8 envelope_volume_r;
    u8 envelope_volume_l;
    u8 echo_volume;
    u8 echo_length;
    u8 unknown1[18];
    u32 frequency;
    u32 wave_address;
    u32 unknown2[6];
  };

  struct SoundInfo {
    u32 magic;
    u8 pcm_dma_counter;
    u8 reverb;
    u8 max_channels;
    u8 master_volume;
    u8 unknown0[8];
    s32 pcm_samples_per_vblank;
    s32 pcm_sample_rate;
    u32 unknown1[14];
    SoundChannel channels[kMaxSoundChannels];
  };

  MP2K(Bus& bus) : bus(bus) {
    Reset();
  }

  bool IsEngaged() const {
    return engaged;
  }

  bool& UseCubicFilter() {
    return use_cubic_filter;
  }

  void Reset();  
  void SoundMainRAM(SoundInfo const& sound_info);
  void RenderFrame();
  auto ReadSample() -> float*;

private:
  static constexpr int kDMABufferSize = 1582;
  static constexpr int kSampleRate = 65536;
  static constexpr int kSamplesPerFrame = kSampleRate / 60 + 1;

  static constexpr float S8ToFloat(s8 value) {
    return value / 127.0;
  }

  struct Sampler {
    bool compressed = false;
    bool should_fetch_sample = true;
    u32 current_position = 0;
    float resample_phase = 0.0;
    float sample_history[4] {0};

    struct WaveInfo {
      u16 type;
      u16 status;
      u32 frequency;
      u32 loop_position;
      u32 number_of_samples;
    } wave_info;

    u8* wave_data = nullptr;
  } samplers[kMaxSoundChannels];

  bool engaged;
  bool use_cubic_filter;
  Bus& bus;
  SoundInfo sound_info;
  std::unique_ptr<float[]> buffer;
  int total_frame_count;
  int current_frame;
  int buffer_read_index;
};

} // namespace nba::core
