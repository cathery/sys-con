#include <switch.h>
#include <variant>
#include "log.h"

#include "SwitchUSBDevice.h"
#include "ControllerHelpers.h"
#include "SwitchHDLHandler.h"
#include "SwitchAbstractedPadHandler.h"
#include "configFile.h"

struct VendorEvent
{
    uint16_t vendor;
    Event event;
};

Result QueryInterfaces(UsbHsInterface *interfaces, size_t interfaces_size, s32 *total_entries, u8 infclass, u8 infsubclass, u8 infprotocol)
{
    UsbHsInterfaceFilter filter;
    filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass | UsbHsInterfaceFilterFlags_bInterfaceProtocol;
    filter.bInterfaceClass = infclass;
    filter.bInterfaceSubClass = infsubclass;
    filter.bInterfaceProtocol = infprotocol;
    Result rc = usbHsQueryAvailableInterfaces(&filter, interfaces, interfaces_size, total_entries);
    if (R_SUCCEEDED(rc) && *total_entries != 0)
        return 0;
    else
        return 1;
}

Result mainLoop()
{
    WriteToLog("\n\nNew sysmodule session started");
    Result rc = 0;
    bool useAbstractedPad = hosversionBetween(5, 7);

    Event xinputEvent;
    Event dinputEvent;
    std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> controllerInterfaces;

    UTimer filecheckTimer;
    Waiter filecheckTimerWaiter = waiterForUTimer(&filecheckTimer);
    utimerCreate(&filecheckTimer, 1e+9L, TimerType_Repeating);
    utimerStart(&filecheckTimer);
    CheckForFileChanges();
    LoadAllConfigs();

    {
        UsbHsInterfaceFilter filter;
        filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass;
        filter.bInterfaceClass = USB_CLASS_VENDOR_SPEC;
        rc = usbHsCreateInterfaceAvailableEvent(&xinputEvent, true, 0, &filter);
        if (R_FAILED(rc))
            WriteToLog("Failed to open event for XInput");
        else
            WriteToLog("Successfully created event for XInput");

        //filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bcdDevice_Min;
        //filter.bInterfaceClass = USB_CLASS_HID;
        //filter.bcdDevice_Min = 0;
        filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
        filter.idVendor = VENDOR_SONY;
        filter.idProduct = PRODUCT_DUALSHOCK3;
        rc = usbHsCreateInterfaceAvailableEvent(&dinputEvent, true, 1, &filter);
        if (R_FAILED(rc))
            WriteToLog("Failed to open event for DInput");
        else
            WriteToLog("Successfully created event for DInput");
    }

    controllerInterfaces.reserve(8);
    std::unique_ptr<IUSBDevice> devicePtr;
    std::unique_ptr<IController> controllerPtr;

    while (appletMainLoop())
    {

#ifdef __APPLET__
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_B)
            break;
#endif
        rc = eventWait(&xinputEvent, 0);
        if (R_SUCCEEDED(rc))
        {
            WriteToLog("XInput went off");
            UsbHsInterface interfaces[8];
            s32 total_entries;

            if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 93, 1)))
            {
                WriteToLog("Registering Xbox 360 controller");
                devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                controllerPtr = std::make_unique<Xbox360Controller>(std::move(devicePtr));
            }
            else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 93, 129)))
            {
                WriteToLog("Registering Xbox 360 Wireless controller");
                devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                controllerPtr = std::make_unique<Xbox360WirelessController>(std::move(devicePtr));
            }
            else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 71, 208)))
            {
                WriteToLog("Registering Xbox One controller");
                devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                controllerPtr = std::make_unique<XboxOneController>(std::move(devicePtr));
            }
        }
        rc = eventWait(&dinputEvent, 0);
        if (R_SUCCEEDED(rc))
        {
            WriteToLog("DInput went off");
            UsbHsInterface interfaces[4];
            s32 total_entries;

            if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_HID, 0, 0)))
            {
                WriteToLog("Registering DS3 controller");
                devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                controllerPtr = std::make_unique<Dualshock3Controller>(std::move(devicePtr));
            }
        }

        if (controllerPtr)
        {
            std::unique_ptr<SwitchVirtualGamepadHandler> switchHandler;
            if (useAbstractedPad)
                switchHandler = std::make_unique<SwitchAbstractedPadHandler>(std::move(controllerPtr));
            else
                switchHandler = std::make_unique<SwitchHDLHandler>(std::move(controllerPtr));

            rc = switchHandler->Initialize();
            if (R_SUCCEEDED(rc))
            {
                controllerInterfaces.push_back(std::move(switchHandler));
                WriteToLog("Interface created successfully");
            }
            else
            {
                WriteToLog("Error creating interface with error ", rc);
            }
        }

        //On interface change event, check if any devices were removed, and erase them from memory appropriately
        rc = eventWait(usbHsGetInterfaceStateChangeEvent(), 0);
        if (R_SUCCEEDED(rc))
        {
            WriteToLog("Interface state was changed");
            eventClear(usbHsGetInterfaceStateChangeEvent());

            UsbHsInterface interfaces[4];
            s32 total_entries;

            rc = usbHsQueryAcquiredInterfaces(interfaces, sizeof(interfaces), &total_entries);
            if (R_SUCCEEDED(rc))
            {
                for (auto it = controllerInterfaces.begin(); it != controllerInterfaces.end(); ++it)
                {
                    bool found_flag = false;

                    for (auto &&ptr : (*it)->GetController()->GetDevice()->GetInterfaces())
                    {
                        //We check if a device was removed by comparing the controller's interfaces and the currently acquired interfaces
                        //If we didn't find a single matching interface ID, we consider a controller removed
                        for (int i = 0; i != total_entries; ++i)
                        {
                            if (interfaces[i].inf.ID == static_cast<SwitchUSBInterface *>(ptr.get())->GetID())
                            {
                                found_flag = true;
                                break;
                            }
                        }
                    }

                    if (!found_flag)
                    {
                        WriteToLog("Erasing controller! ", (*it)->GetController()->GetType());
                        controllerInterfaces.erase(it--);
                        WriteToLog("Controller erased!");
                    }
                }
            }
        }

        rc = waitSingle(filecheckTimerWaiter, 0);
        if (R_SUCCEEDED(rc))
        {
            if (CheckForFileChanges())
            {
                WriteToLog("File check succeeded! Loading configs...");
                LoadAllConfigs();
            }
        }

#ifdef __APPLET__
        consoleUpdate(nullptr);
#else
        svcSleepThread(1e+7L);
#endif
    }

    //After we break out of the loop, close all events and exit
    WriteToLog("Destroying events");
    usbHsDestroyInterfaceAvailableEvent(&xinputEvent, 0);
    usbHsDestroyInterfaceAvailableEvent(&dinputEvent, 1);

    //controllerInterfaces.clear();
    return rc;
}