#pragma once
#include "Status.h"
#include <cstddef>

class IUSBEndpoint
{
public:
    enum Direction
    {
        USB_ENDPOINT_IN = 0x80,
        USB_ENDPOINT_OUT = 0x00,
    };

    struct EndpointDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bEndpointAddress;
        uint8_t bmAttributes;
        uint16_t wMaxPacketSize;
        uint8_t bInterval;
    };

    virtual ~IUSBEndpoint() = default;

    //Open and close the endpoint.
    virtual Status Open() = 0;
    virtual void Close() = 0;

    //This will read from the inBuffer pointer for the specified size and write it to the endpoint.
    virtual Status Write(const void *inBuffer, size_t bufferSize) = 0;

    //This will read from the endpoint and put the data in the outBuffer pointer for the specified size.
    virtual Status Read(void *outBuffer, size_t bufferSize) = 0;

    //Get endpoint's direction. (IN or OUT)
    virtual IUSBEndpoint::Direction GetDirection() = 0;
    //Get the endpoint descriptor
    virtual EndpointDescriptor *GetDescriptor() = 0;
};