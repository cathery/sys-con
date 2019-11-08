#pragma once
#include <cstdio>
#include "Status.h"
#include "IUSBInterface.h"
#include <memory>
#include <vector>

class IUSBDevice
{
protected:
  std::vector<std::unique_ptr<IUSBInterface>> m_interfaces{};

  uint16_t m_vendorID;
  uint16_t m_productID;

public:
  virtual ~IUSBDevice() = default;

  //Open and close the device.
  virtual Status Open() = 0;
  virtual void Close() = 0;

  //Reset the device.
  virtual void Reset() = 0;

  //Get the raw reference to interfaces vector.
  virtual std::vector<std::unique_ptr<IUSBInterface>> &GetInterfaces() { return m_interfaces; }

  virtual uint16_t GetVendor() { return m_vendorID; }
  virtual uint16_t GetProduct() { return m_productID; }
};