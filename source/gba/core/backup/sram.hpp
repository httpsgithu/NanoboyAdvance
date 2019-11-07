/**
  * Copyright (C) 2019 fleroviux (Frederic Meyer)
  *
  * This file is part of NanoboyAdvance.
  *
  * NanoboyAdvance is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * NanoboyAdvance is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with NanoboyAdvance. If not, see <http://www.gnu.org/licenses/>.
  */

#pragma once

#include <string>

#include "backup.hpp"
#include "backup_file.hpp"

namespace GameBoyAdvance {

class SRAM : public Backup {

public:
  SRAM(std::string const& save_path)
    : save_path(save_path) {
    Reset();
  }
  
  void Reset() final {
    int bytes = 32768;
    file = BackupFile::OpenOrCreate(save_path, { 32768 }, bytes);
  }
  
  auto Read(std::uint32_t address) -> std::uint8_t final {
    return file->Read(address & 0x7FFF);
  }
  
  void Write(std::uint32_t address, std::uint8_t value) final {
    file->Write(address & 0x7FFF, value);
  }
  
private:
  std::string save_path;
  std::unique_ptr<BackupFile> file;
};

}