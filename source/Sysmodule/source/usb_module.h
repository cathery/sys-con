#pragma once

namespace syscon::usb {

    void Initialize();
    void Exit();

    Result Enable();
    void Disable();

    Result CreateUsbEvents();
    void DestroyUsbEvents();

    Result ReloadDualshock4Event();
}