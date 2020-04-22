#pragma once
#include "IController.h"

//References used:
//https://cs.chromium.org/chromium/src/device/gamepad/dualshock4_controller.cc

struct Dualshock4ButtonData
{
    uint8_t padding0[2];

    uint8_t stick_left_x;
    uint8_t stick_left_y;
    uint8_t stick_right_x;
    uint8_t stick_right_y;
    uint8_t dpad : 4;
    bool square : 1;
    bool cross : 1;
    bool circle : 1;
    bool triangle : 1;
    bool l1 : 1;
    bool r1 : 1;
    bool l2 : 1;
    bool r2 : 1;
    bool share : 1;
    bool options : 1;
    bool l3 : 1;
    bool r3 : 1;
    bool psbutton : 1;
    bool touchpad_press : 1;
    uint8_t sequence_number : 6;
    uint8_t l2_pressure;
    uint8_t r2_pressure;
    uint16_t timestamp;
    uint8_t sensor_temperature;
    uint16_t gyro_pitch;
    uint16_t gyro_yaw;
    uint16_t gyro_roll;
    uint16_t accelerometer_x;
    uint16_t accelerometer_y;
    uint16_t accelerometer_z;
    uint8_t padding1[5];
    uint8_t battery_info : 5;
    uint8_t padding2 : 2;
    bool extension_detection : 1;

    uint8_t padding3[2];

    uint8_t touches_count;
    uint8_t touch_data_timestamp;
    uint8_t touch0_id : 7;
    bool touch0_is_invalid : 1;
    uint8_t touch0_data[3];
    uint8_t touch1_id : 7;
    bool touch1_is_invalid : 1;
    uint8_t touch1_data[3];

    uint8_t padding4[2];
    uint32_t crc32;
};

struct Dualshock4USBButtonData
{
    uint8_t type;
    uint8_t stick_left_x;
    uint8_t stick_left_y;
    uint8_t stick_right_x;
    uint8_t stick_right_y;

    uint8_t dpad : 4;
    bool square : 1;
    bool cross : 1;
    bool circle : 1;
    bool triangle : 1;

    bool l1 : 1;
    bool r1 : 1;
    bool l2 : 1;
    bool r2 : 1;
    bool share : 1;
    bool options : 1;
    bool l3 : 1;
    bool r3 : 1;

    bool psbutton : 1;
    bool touchpad_press : 1;
    uint8_t timestamp : 6;

    uint8_t l2_pressure;
    uint8_t r2_pressure;

    uint8_t counter[2];

    uint8_t battery_level;

    uint8_t unk;

    uint16_t gyroX;
    uint16_t gyroY;
    uint16_t gyroZ;

    uint16_t accelX;
    uint16_t accelY;
    uint16_t accelZ;

    uint32_t unk2;

    uint8_t headset_bitmask;

    uint16_t unk3;

    uint8_t touchpad_eventData : 4;
    uint8_t unk4 : 4;

    uint8_t touchpad_counter;
    uint8_t touchpad_finger1_counter : 7;
    bool touchpad_finger1_unpressed : 1;
    uint16_t touchpad_finger1_X;
    uint8_t touchpad_finger1_Y;

    uint8_t touchpad_finger2_counter : 7;
    bool touchpad_finger2_unpressed : 1;
    uint16_t touchpad_finger2_X;
    uint8_t touchpad_finger2_Y;

    uint8_t touchpad_finger1_prev[4];
    uint8_t touchpad_finger2_prev[4];
};

enum Dualshock4Dpad : uint8_t
{
    DS4_UP,
    DS4_UPRIGHT,
    DS4_RIGHT,
    DS4_DOWNRIGHT,
    DS4_DOWN,
    DS4_DOWNLEFT,
    DS4_LEFT,
    DS4_UPLEFT,
};

class Dualshock4Controller : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

    Dualshock4USBButtonData m_buttonData{};

public:
    Dualshock4Controller(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~Dualshock4Controller() override;

    virtual Result Initialize() override;
    virtual void Exit() override;

    Result OpenInterfaces();
    void CloseInterfaces();

    virtual Result GetInput() override;

    virtual NormalizedButtonData GetNormalizedButtonData() override;

    virtual ControllerType GetType() override { return CONTROLLER_DUALSHOCK4; }

    float NormalizeTrigger(uint8_t deadzonePercent, uint8_t value);
    void NormalizeAxis(uint8_t x, uint8_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Result SendInitBytes();
    Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    static void LoadConfig(const ControllerConfig *config, RGBAColor ledValue);
    virtual ControllerConfig *GetConfig() override;
};