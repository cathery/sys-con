#pragma once
#include <cstdint>
enum ControllerButton
{
    NOT_SET = -1,
    FACE_UP,
    FACE_RIGHT,
    FACE_DOWN,
    FACE_LEFT,
    LSTICK,
    RSTICK,
    LSTICK_CLICK,
    RSTICK_CLICK,
    LEFT_BUMPER,
    RIGHT_BUMPER,
    LEFT_TRIGGER,
    RIGHT_TRIGGER,
    BACK,
    START,
    DPAD_UP,
    DPAD_RIGHT,
    DPAD_DOWN,
    DPAD_LEFT,
    SYNC,
    GUIDE,
};

struct ControllerConfig
{
    uint16_t leftStickDeadzone;
    uint16_t rightStickDeadzone;
    uint16_t leftStickRotation;
    uint16_t rightStickRotation;
    uint16_t triggerDeadzone;
    ControllerButton buttons[20];
};