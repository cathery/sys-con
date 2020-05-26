#pragma once

#ifdef __cplusplus
#include "IController.h"
#include "Dualshock4Controller.h"

extern "C" {
#endif

void registerNetworkController(int fd);
void removeNetworkController(int fd);

#ifdef __cplusplus
}

class NetworkController : public IController
{
private:
    int m_fd;
    Dualshock4USBButtonData m_buttonData{};
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
