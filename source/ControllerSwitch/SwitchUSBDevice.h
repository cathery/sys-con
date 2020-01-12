#pragma once
#include "switch.h"
#include "IUSBDevice.h"
#include "SwitchUSBInterface.h"
#include <vector>

class SwitchUSBDevice : public IUSBDevice
{
public:
    SwitchUSBDevice();
    ~SwitchUSBDevice();

    //Initialize the class with the SetInterfaces call.
    SwitchUSBDevice(UsbHsInterface *interfaces, int length);

    //There are no devices to open on the switch, so instead this returns success if there are any interfaces
    virtual Result Open() override;
    //Closes all the interfaces associated with the class
    virtual void Close() override;

    // Resets the device
    virtual void Reset() override;

    //Create interfaces from the given array of specified element length
    void SetInterfaces(UsbHsInterface *interfaces, int length);
};