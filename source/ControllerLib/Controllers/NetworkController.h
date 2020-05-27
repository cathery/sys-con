#pragma once

#ifdef __cplusplus
#include "IController.h"

extern "C" {
#endif

void registerNetworkController(int fd);
void removeNetworkController(int fd);

#ifdef __cplusplus
}

struct NetworkButtonData
{
    uint8_t stick_left_x;
    uint8_t stick_left_y;
    uint8_t stick_right_x;
    uint8_t stick_right_y;

    bool dleft : 1;
    bool dup : 1;
    bool dright : 1;
    bool ddown : 1;
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
};

class NetworkController : public IController
{
private:
    int m_fd;
    NetworkButtonData m_buttonData{};
    void DeleteNetworkController();
public:
    NetworkController(int fd);
    virtual ~NetworkController() override;

    virtual Result Initialize() override;
    virtual void Exit() override;

    Result OpenInterfaces();
    void CloseInterfaces();

    virtual Result GetInput() override;

    virtual NormalizedButtonData GetNormalizedButtonData() override;

    virtual ControllerType GetType() override { return CONTROLLER_NETWORK; }

    float NormalizeTrigger(uint8_t deadzonePercent, uint8_t value);
    void NormalizeAxis(uint8_t x, uint8_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Result SendInitBytes();
    Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    static void LoadConfig(const ControllerConfig *config, RGBAColor ledValue);
    virtual ControllerConfig *GetConfig() override;

    int GetFD() { return m_fd; }
};
#endif
