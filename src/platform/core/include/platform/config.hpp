/*
 * Copyright (C) 2024 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <nba/config.hpp>
#include <string>
#include <toml.hpp>

namespace nba {

struct PlatformConfig : Config {
  std::string bios_path = "bios.bin";
  std::string save_folder = "";
  
  struct Cartridge {
    BackupType backup_type = BackupType::Detect;
    bool force_rtc = true;
    bool force_solar_sensor = false;
    u8 solar_sensor_level = 23;
  } cartridge;

  struct Video {
    enum class Filter {
      Nearest,
      Linear,
      Sharp,
      xBRZ,
      Lcd1x
    } filter = Filter::Linear;

    enum class Color {
      No,
      higan,
      AGB
    } color = Color::AGB;

    bool lcd_ghosting = true;
  } video;

  void Load(std::string const& path);
  void Save(std::string const& path);

protected:
  virtual void LoadCustomData(toml::value const& data) {}
  virtual void SaveCustomData(
    toml::basic_value<toml::preserve_comments>& data
  ) {};
};

} // namespace nba
