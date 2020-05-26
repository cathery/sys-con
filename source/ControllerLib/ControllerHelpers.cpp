#include "ControllerHelpers.h"

bool DoesControllerSupport(ControllerType type, ControllerSupport supportType)
{
    switch (type)
    {
    case CONTROLLER_XBOX360:
        if (supportType == SUPPORTS_RUMBLE)
            return true;
        return false;
    case CONTROLLER_XBOX360W:
        if (supportType == SUPPORTS_PAIRING)
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
    case CONTROLLER_XBOXONEW:
        if (supportType == SUPPORTS_NOTHING)
            return true;
        return false;
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
    case CONTROLLER_DUALSHOCK4:
        switch (supportType)
        {
        case SUPPORTS_RUMBLE:
            return true;
        case SUPPORTS_BLUETOOTH:
            return true;
        case SUPPORTS_SIXAXIS:
            return true;
        default:
            return false;
        }
    case CONTROLLER_NETWORK:
        switch (supportType)
        {
        case SUPPORTS_RUMBLE:
            return true;
        case SUPPORTS_BLUETOOTH:
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
