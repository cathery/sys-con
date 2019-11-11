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

enum Dualshock4Dpad
{
    DS4_UP = 0x000,
    DS4_UPRIGHT = 0x001,
    DS4_RIGHT = 0x010,
    DS4_DOWNRIGHT = 0x011,
    DS4_DOWN = 0x100,
    DS4_DOWNLEFT = 0x101,
    DS4_LEFT = 0x110,
    DS4_UPLEFT = 0x111,
};

class Dualshock4Controller : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

    Dualshock4ButtonData m_buttonData;

public:
    Dualshock4Controller(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~Dualshock4Controller();

    virtual Status Initialize();
    virtual void Exit();

    Status OpenInterfaces();
    void CloseInterfaces();

    virtual Status GetInput();

    virtual NormalizedButtonData GetNormalizedButtonData();

    virtual ControllerType GetType() { return CONTROLLER_DUALSHOCK4; }

    inline const Dualshock4ButtonData &GetButtonData() { return m_buttonData; };

    float NormalizeTrigger(uint16_t value);
    void NormalizeAxis(int16_t x, int16_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Status SendInitBytes();
    Status SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    static void LoadConfig(const ControllerConfig *config);
    virtual ControllerConfig *GetConfig();
};