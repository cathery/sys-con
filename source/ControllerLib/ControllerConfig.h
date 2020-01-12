#pragma once
#include <cstdint>
enum ControllerButton : int8_t
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
    TOUCHPAD,
    NUM_CONTROLLERBUTTONS,
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
    uint8_t leftStickDeadzonePercent{10};
    uint8_t rightStickDeadzonePercent{10};
    uint16_t leftStickRotationDegrees{0};
    uint16_t rightStickRotationDegrees{0};
    uint8_t triggerDeadzonePercent{0};
    ControllerButton buttons[NUM_CONTROLLERBUTTONS];
    float triggers[2]{0};
    NormalizedStick sticks[2]{0};
    bool swapDPADandLSTICK{false};
    RGBAColor bodyColor{107, 107, 107, 255};
    RGBAColor buttonsColor{0, 0, 0, 255};
    RGBAColor leftGripColor{70, 70, 70, 255};
    RGBAColor rightGripColor{70, 70, 70, 255};
};