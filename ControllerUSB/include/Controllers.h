#pragma once
#include <map>
//Catch-all header to include all the controllers
#include "Controllers/Xbox360Controller.h"
#include "Controllers/XboxOneController.h"

std::vector<uint16_t> GetVendors()
{
    return {VENDOR_MICROSOFT};
}

std::vector<uint16_t> GetVendorProducts(uint16_t vendor_id)
{
    switch (vendor_id)
    {
    case VENDOR_MICROSOFT:
        return {PRODUCT_XBOX360,
                PRODUCT_XBOXONE2013,
                PRODUCT_XBOXONE2015,
                PRODUCT_XBOXONEELITE,
                PRODUCT_XBOXONES,
                PRODUCT_XBOXADAPTIVE};
    }
    return {};
}

std::unique_ptr<IController> ConstructControllerFromType(ControllerType type, std::unique_ptr<IUSBDevice> &&device)
{

    //surely there must be a better way to pass a class type from a function
    switch (type)
    {
    case CONTROLLER_XBOX360:
        return std::make_unique<Xbox360Controller>(std::move(device));
    case CONTROLLER_XBOXONE:
        return std::make_unique<XboxOneController>(std::move(device));
    default:
        break;
    }
    return std::unique_ptr<IController>{};
}

ControllerType GetControllerTypeFromIds(uint16_t vendor_id, uint16_t product_id)
{
    switch (vendor_id)
    {
    case VENDOR_MICROSOFT:
        switch (product_id)
        {
        case PRODUCT_XBOX360:
            return CONTROLLER_XBOX360;
        case PRODUCT_XBOXONE2013:
        case PRODUCT_XBOXONE2015:
        case PRODUCT_XBOXONEELITE:
        case PRODUCT_XBOXONES:
        case PRODUCT_XBOXADAPTIVE:
            return CONTROLLER_XBOXONE;
        }
        break;
    default:
        break;
    }
    return CONTROLLER_UNDEFINED;
}