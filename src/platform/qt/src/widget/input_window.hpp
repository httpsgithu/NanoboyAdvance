/*
 * Copyright (C) 2024 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <nba/core.hpp>
#include <nba/log.hpp>
#include <toml.hpp>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QPushButton>
#include <QLabel>

#include "config.hpp"

inline auto GetJoystickGUIDStringFromIndex(int device_index) -> std::string {
  auto guid = SDL_JoystickGetDeviceGUID(device_index);
  auto guid_string = std::string{};

  guid_string.resize(sizeof(SDL_JoystickGUID) * 2);
  SDL_JoystickGetGUIDString(guid, guid_string.data(), guid_string.size() + 1);
  return guid_string;
}

struct InputWindow : QDialog {
  using Key = nba::Key;

  InputWindow(
    QApplication* app,
    QWidget* parent,
    std::shared_ptr<QtConfig> config
  );

  void BindCurrentKeyToJoystickButton(int button);
  void BindCurrentKeyToJoystickAxis(int axis, bool negative);
  void BindCurrentKeyToJoystickHat(int hat, int direction);

  void UpdateJoystickList();

  std::atomic_bool has_game_controller_choice_changed = false;

protected:
  bool eventFilter(QObject* obj, QEvent* event);

private:
  auto CreateJoystickList() -> QLayout*;
  auto CreateKeyMapTable() -> QLayout*;

  void CreateKeyMapEntry(
    QGridLayout* layout,
    const char* label,
    QtConfig::Input::Map* mapping
  );

  void RestoreActiveButtonLabel();

  static auto GetKeyboardButtonName(int key) -> QString;
  static auto GetJoystickButtonName(QtConfig::Input::Map* mapping) -> QString;

  bool waiting_for_keyboard = false;
  bool waiting_for_joystick = false;
  QtConfig::Input::Map* active_mapping = nullptr;
  QPushButton* active_button = nullptr;
  QComboBox* joystick_combo_box;
  std::shared_ptr<QtConfig> config;
};