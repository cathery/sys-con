#pragma once
#include <cstdint>

#define MAX_JOYSTICKS          2
#define MAX_TRIGGERS           2
#define MAX_CONTROLLER_BUTTONS 32

enum ControllerButton : uint8_t
{
    DEFAULT = 0,
    NONE,
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
    CAPTURE,
    HOME,
    SYNC,
    TOUCHPAD,
};

struct NormalizedStick
{
    float axis_x;
    float axis_y;
};

union RGBAColor {
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    uint8_t values[4];
    uint32_t rgbaValue;
};

struct ControllerConfig
{
    uint8_t stickDeadzonePercent[MAX_JOYSTICKS]{};
    uint16_t stickRotationDegrees[MAX_JOYSTICKS]{};
    uint8_t triggerDeadzonePercent[MAX_TRIGGERS]{};
    ControllerButton buttons[MAX_CONTROLLER_BUTTONS]{};
    float triggers[MAX_TRIGGERS]{};
    NormalizedStick sticks[MAX_JOYSTICKS]{};
    bool swapDPADandLSTICK{false};
    RGBAColor bodyColor{107, 107, 107, 255};
    RGBAColor buttonsColor{0, 0, 0, 255};
    RGBAColor leftGripColor{70, 70, 70, 255};
    RGBAColor rightGripColor{70, 70, 70, 255};
};