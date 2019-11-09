#pragma once
#include <cstdint>
enum ControllerButton
{
    NOT_SET = -1,
    FACE_UP,
    FACE_RIGHT,
    FACE_DOWN,
    FACE_LEFT,
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
    NUM_CONTROLLERBUTTONS,
};

struct NormalizedStick
{
    float axis_x;
    float axis_y;
};

struct ControllerConfig
{
    uint8_t leftStickDeadzonePercent;
    uint8_t rightStickDeadzonePercent;
    uint16_t leftStickRotationDegrees;
    uint16_t rightStickRotationDegrees;
    uint8_t triggerDeadzonePercent;
    ControllerButton buttons[NUM_CONTROLLERBUTTONS];
    float triggers[2];
    NormalizedStick sticks[2];
    bool swapDPADandLSTICK;
};