/*
 * Copyright (C) 2024 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "main_window.hpp"
#include "controller_manager.hpp"

ControllerManager::~ControllerManager() {
  if(timer) {
    timer->stop();
    Close();
  } else {
    quitting = true;
    thread.join();
  }
}

void ControllerManager::Initialize() {
  SDL_Init(SDL_INIT_JOYSTICK);

  /* On macOS we may not poll SDL events on a separate thread.
   * So what we do instead is handle device connect/remove events
   * from a 100 ms Qt timer and updating the input from the emulator thread each frame.
   */
#if defined(__APPLE__)
  timer = new QTimer{this};
  timer->start(100);
  connect(timer, &QTimer::timeout, std::bind(&ControllerManager::ProcessEvents, this));
  main_window->emu_thread->SetPerFrameCallback(
    std::bind(&ControllerManager::UpdateKeyState, this));
#else
  thread = std::thread{[this]() {
    // SDL_WaitEventTimeout() requires video to be initialised on the same thread.
    SDL_Init(SDL_INIT_VIDEO);

    while(!quitting) {
      SDL_WaitEventTimeout(nullptr, 100);

      ProcessEvents();
      UpdateKeyState();
    }

    Close();
  }};
#endif

  connect(
    this, &ControllerManager::OnJoystickListChanged,
    this, &ControllerManager::UpdateJoystickList,
    Qt::QueuedConnection
  );

  connect(
    this, &ControllerManager::OnJoystickButtonReleased,
    this, &ControllerManager::BindCurrentKeyToJoystickButton,
    Qt::QueuedConnection
  );

  connect(
    this, &ControllerManager::OnJoystickAxisMoved,
    this, &ControllerManager::BindCurrentKeyToJoystickAxis,
    Qt::QueuedConnection
  );

  connect(
    this, &ControllerManager::OnJoystickHatMoved,
    this, &ControllerManager::BindCurrentKeyToJoystickHat,
    Qt::QueuedConnection
  );
}

void ControllerManager::UpdateJoystickList() {
  auto input_window = main_window->input_window;

  if(input_window) {
    input_window->UpdateJoystickList();
  }
}

void ControllerManager::BindCurrentKeyToJoystickButton(int button) {
  auto input_window = main_window->input_window;

  if(input_window) {
    input_window->BindCurrentKeyToJoystickButton(button);
  }
}

void ControllerManager::BindCurrentKeyToJoystickAxis(int axis, bool negative) {
  auto input_window = main_window->input_window;

  if(input_window) {
    input_window->BindCurrentKeyToJoystickAxis(axis, negative);
  }
}

void ControllerManager::BindCurrentKeyToJoystickHat(int hat, int direction) {
  auto input_window = main_window->input_window;

  if(input_window) {
    input_window->BindCurrentKeyToJoystickHat(hat, direction);
  } 
}

void ControllerManager::Open(std::string const& guid) {
  auto joystick_count = SDL_NumJoysticks();

  Close();

  for(int device_id = 0; device_id < joystick_count; device_id++) {
    if(GetJoystickGUIDStringFromIndex(device_id) == guid) {
      joystick = SDL_JoystickOpen(device_id);
      instance_id = SDL_JoystickInstanceID(joystick);
      break;
    }
  }
}

void ControllerManager::Close() {
  using Key = nba::Key;

  // Unset all keys in case any key is currently pressed.
  main_window->SetKeyStatus(1, Key::Up, false);
  main_window->SetKeyStatus(1, Key::Down, false);
  main_window->SetKeyStatus(1, Key::Left, false);
  main_window->SetKeyStatus(1, Key::Right, false);
  main_window->SetKeyStatus(1, Key::Start, false);
  main_window->SetKeyStatus(1, Key::Select, false);
  main_window->SetKeyStatus(1, Key::A, false);
  main_window->SetKeyStatus(1, Key::B, false);
  main_window->SetKeyStatus(1, Key::L, false);
  main_window->SetKeyStatus(1, Key::R, false);

  if(joystick) {
    SDL_JoystickClose(joystick);
    joystick = nullptr;
  }
}

void ControllerManager::ProcessEvents() {
  auto event = SDL_Event{};
  auto input_window = main_window->input_window;

#if defined(__APPLE__)
  std::lock_guard guard{lock};
#endif

  while(SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_JOYDEVICEADDED: {
        emit OnJoystickListChanged();

        auto device_id = ((SDL_JoyDeviceEvent*)&event)->which;
        auto guid = GetJoystickGUIDStringFromIndex(device_id);

        if(guid == config->input.controller_guid) {
          Open(guid);
        }
        break;
      }
      case SDL_JOYDEVICEREMOVED: {
        emit OnJoystickListChanged();

        if(instance_id == ((SDL_JoyDeviceEvent*)&event)->which) {
          Close();
        }
        break;
      }
      case SDL_JOYBUTTONUP: {
        auto button_event = (SDL_JoyButtonEvent*)&event;

        emit OnJoystickButtonReleased(button_event->button);
        break;
      }
      case SDL_JOYAXISMOTION: {
        const auto threshold = std::numeric_limits<int16_t>::max() / 2;

        auto axis_event = (SDL_JoyAxisEvent*)&event;
        auto value = axis_event->value;

        if(std::abs(value) > threshold) {
          emit OnJoystickAxisMoved(axis_event->axis, value < 0);
        }
        break;
      }
      case SDL_JOYHATMOTION: {
        const auto hat_event = (SDL_JoyHatEvent*)&event;

        /**
         * Make sure to pick a single direction when the hat is triggered in
         * two directions at once (i.e. left + up).
         * To achieve this we extract the lowest set bit with some bit magic.
         */
        const int direction = hat_event->value & ~(hat_event->value - 1);

        if(direction != 0) {
          emit OnJoystickHatMoved(hat_event->hat, direction);
        }
        break;
      }
    }
  }

  if(input_window && input_window->has_game_controller_choice_changed) {
    Open(config->input.controller_guid);

    input_window->has_game_controller_choice_changed = false;
  }
}

void ControllerManager::UpdateKeyState() {
  using Key = nba::Key;

#if defined(__APPLE__)
  std::lock_guard guard{lock};
#endif

  if(joystick == nullptr) {
    return;
  }

  auto const& input = config->input;

  const auto evaluate = [&](QtConfig::Input::Map const& mapping) {
    bool pressed = false;
  
    const int button = mapping.controller.button;
    const int axis = mapping.controller.axis;
    const int hat = mapping.controller.hat;

    if(button != -1) {
      pressed = pressed || SDL_JoystickGetButton(joystick, button);
    }

    if(axis != -1) {
      const auto threshold = std::numeric_limits<int16_t>::max() / 2;

      auto actual_axis = axis & ~0x100;
      bool negative = axis & 0x100;
      auto value = SDL_JoystickGetAxis(joystick, actual_axis);

      pressed = pressed || (negative ? (value < -threshold) : (value > threshold));
    }

    if(hat != -1) {
      const int hat_direction = mapping.controller.hat_direction;

      pressed = pressed || ((SDL_JoystickGetHat(joystick, hat) & hat_direction) != 0);
    }

    return pressed;
  };

  for(int i = 0; i < (int)nba::Key::Count; i++) {
    main_window->SetKeyStatus(
      1, static_cast<nba::Key>(i), evaluate(input.gba[i]));
  }

  bool fast_forward_button = evaluate(input.fast_forward);

  if(fast_forward_button != fast_forward_button_old) {
    main_window->SetFastForward(1, fast_forward_button);
    fast_forward_button_old = fast_forward_button;
  }
}