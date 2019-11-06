#pragma once
#include "Controllers.h"

//Returns a vector with all vendor IDs
std::vector<uint16_t> GetVendors();

//Returns all product IDs for specified vendor
std::vector<uint16_t> GetVendorProducts(uint16_t vendor_id);

//Returns a constructed controller derived from IController based on the type
std::unique_ptr<IController> ConstructControllerFromType(ControllerType type, std::unique_ptr<IUSBDevice> &&device);

//Gets the controller type based on vendor + product combo
ControllerType GetControllerTypeFromIds(uint16_t vendor_id, uint16_t product_id);

//Returns true if said controller supports said feature
bool DoesControllerSupport(ControllerType type, ControllerSupport supportType);