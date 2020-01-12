#pragma once
#include "Result.h"
#include "IUSBEndpoint.h"
#include <memory>

class IUSBInterface
{
protected:
public:
    struct InterfaceDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;   ///< Must match USB_DT_INTERFACE.
        uint8_t bInterfaceNumber;  ///< See also USBDS_DEFAULT_InterfaceNumber.
        uint8_t bAlternateSetting; ///< Must match 0.
        uint8_t bNumEndpoints;
        uint8_t bInterfaceClass;
        uint8_t bInterfaceSubClass;
        uint8_t bInterfaceProtocol;
        uint8_t iInterface; ///< Ignored.
    };
    virtual ~IUSBInterface() = default;

    virtual Result Open() = 0;
    virtual void Close() = 0;

    virtual Result ControlTransfer(uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, void *buffer) = 0;
    virtual Result ControlTransfer(uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const void *buffer) = 0;

    virtual IUSBEndpoint *GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index) = 0;
    virtual InterfaceDescriptor *GetDescriptor() = 0;

    virtual Result Reset() = 0;
};