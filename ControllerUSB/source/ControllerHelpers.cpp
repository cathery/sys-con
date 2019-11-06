#include "ControllerHelpers.h"

std::vector<uint16_t> GetVendors()
{
    return {VENDOR_MICROSOFT, VENDOR_SONY};
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
    case VENDOR_SONY:
        return {PRODUCT_DUALSHOCK3,
                PRODUCT_DUALSHOCK4};
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
    case CONTROLLER_DUALSHOCK3:
        return std::make_unique<Dualshock3Controller>(std::move(device));
    case CONTROLLER_DUALSHOCK4:
        return std::make_unique<Dualshock4Controller>(std::move(device));
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
    case VENDOR_SONY:
        switch (product_id)
        {
        case PRODUCT_DUALSHOCK3:
            return CONTROLLER_DUALSHOCK3;
        case PRODUCT_DUALSHOCK4:
            return CONTROLLER_DUALSHOCK4;
        }
        break;
    default:
        break;
    }
    return CONTROLLER_UNDEFINED;
}

bool DoesControllerSupport(ControllerType type, ControllerSupport supportType)
{
    switch (type)
    {
    case CONTROLLER_XBOX360:
        if (supportType == SUPPORTS_RUMBLE)
            return true;
        return false;
    case CONTROLLER_XBOXONE:
        switch (supportType)
        {
        case SUPPORTS_RUMBLE:
            return true;
        case SUPPORTS_BLUETOOTH:
            return true;
        default:
            return false;
        }
    case CONTROLLER_DUALSHOCK3:
        switch (supportType)
        {
        case SUPPORTS_RUMBLE:
            return true;
        case SUPPORTS_BLUETOOTH:
            return true;
        case SUPPORTS_PRESSUREBUTTONS:
            return true;
        case SUPPORTS_SIXAXIS:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
    return false;
}