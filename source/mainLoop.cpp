#include <switch.h>
#include <variant>
#include "log.h"

#include "SwitchUSBDevice.h"
#include "ControllerHelpers.h"
#include "SwitchHDLHandler.h"
#include "SwitchAbstractedPadHandler.h"
#include "configFile.h"

#define APP_VERSION "0.4.3"

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

Result QueryVendorProduct(UsbHsInterface *interfaces, size_t interfaces_size, s32 *total_entries, uint16_t vendor_id, uint16_t product_id)
{
    UsbHsInterfaceFilter filter;
    filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
    filter.idVendor = vendor_id;
    filter.idProduct = product_id;
    Result rc = usbHsQueryAvailableInterfaces(&filter, interfaces, interfaces_size, total_entries);
    if (R_SUCCEEDED(rc) && *total_entries != 0)
        return 0;
    else
        return 1;
}

std::unique_ptr<IUSBDevice> devicePtr;
std::unique_ptr<IController> controllerPtr;
bool useAbstractedPad = hosversionBetween(5, 7);
std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> controllerInterfaces;

Result CallInitHandler()
{
    if (controllerPtr)
    {
        Result rc;
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
            return 0;
        }
        else
        {
            WriteToLog("Error creating interface with error ", rc);
            return rc;
        }
    }
    return 1;
}

Result mainLoop()
{
    WriteToLog("\n\nNew sysmodule session started on version " APP_VERSION);
    Result rc = 0;

    Event catchAllEvent;
    Event ds3Event;
    Event ds4Event;

    UTimer filecheckTimer;
    Waiter filecheckTimerWaiter = waiterForUTimer(&filecheckTimer);
    utimerCreate(&filecheckTimer, 1e+9L, TimerType_Repeating);
    utimerStart(&filecheckTimer);
    CheckForFileChanges();
    LoadAllConfigs();

    {
        UsbHsInterfaceFilter filter;
        //filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bcdDevice_Min;
        //filter.bInterfaceClass = USB_CLASS_HID;
        //filter.bcdDevice_Min = 0;
        filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
        filter.idVendor = VENDOR_SONY;
        filter.idProduct = PRODUCT_DUALSHOCK3;
        rc = usbHsCreateInterfaceAvailableEvent(&ds3Event, true, 0, &filter);
        if (R_FAILED(rc))
            WriteToLog("Failed to open event for Dualshock 3");
        else
            WriteToLog("Successfully created event for Dualshock 3");

        filter.Flags = UsbHsInterfaceFilterFlags_bcdDevice_Min;
        filter.bcdDevice_Min = 0;
        rc = usbHsCreateInterfaceAvailableEvent(&catchAllEvent, true, 1, &filter);
        if (R_FAILED(rc))
            WriteToLog("Failed to open catch-all event");
        else
            WriteToLog("Successfully created catch-all event");

        filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
        filter.idVendor = VENDOR_SONY;
        filter.idProduct = PRODUCT_DUALSHOCK4_2X;
        rc = usbHsCreateInterfaceAvailableEvent(&ds4Event, true, 2, &filter);
        if (R_FAILED(rc))
            WriteToLog("Failed to open event for Dualshock 4 2x");
        else
            WriteToLog("Successfully created event for Dualshock 4 2x");
        /*
        filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
        filter.idVendor = VENDOR_SONY;
        filter.idProduct = PRODUCT_DUALSHOCK4_1X;
        rc = usbHsCreateInterfaceAvailableEvent(&ds4Event, true, 2, &filter);
        if (R_FAILED(rc))
            WriteToLog("Failed to open event for Dualshock 4 1x");
        else
            WriteToLog("Successfully created event for Dualshock 4 1x");
        */
    }

    controllerInterfaces.reserve(10);

    while (appletMainLoop())
    {

#ifdef __APPLET__
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        for (int i = 0; i != 8; ++i)
        {
            u64 kHeld = hidKeysDown(static_cast<HidControllerID>(i));
            if (kHeld != 0)
                WriteToLog("Player ", i + 1, ": ", kHeld);
        }

        if (kDown & KEY_B)
            break;

        for (auto &&handler : controllerInterfaces)
        {
            if (handler->GetController()->m_UpdateCalled)
            {
                for (int i = 0; i != 64; ++i)
                    printf("0x%02X ", handler->GetController()->m_inputData[i]);
                printf("\n");
                handler->GetController()->m_UpdateCalled = false;
            }
        }
#endif
        rc = eventWait(&catchAllEvent, 0);
        if (R_SUCCEEDED(rc))
        {
            WriteToLog("Catch-all event went off");
            if (controllerInterfaces.size() >= 10)
                WriteToLog("But the controllers table reached its max size!");
            else
            {

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
                    WriteToLog("Registering Xbox 360 Wireless adapter");
                    for (int i = 0; i != total_entries; ++i)
                    {
                        devicePtr = std::make_unique<SwitchUSBDevice>(interfaces + i, 1);
                        controllerPtr = std::make_unique<Xbox360WirelessController>(std::move(devicePtr));
                        CallInitHandler();
                    }
                }
                else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, 0x58, 0x42, 0x00)))
                {
                    WriteToLog("Registering Xbox One controller");
                    devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                    controllerPtr = std::make_unique<XboxController>(std::move(devicePtr));
                }
                else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 71, 208)))
                {
                    WriteToLog("Registering Xbox One controller");
                    devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                    controllerPtr = std::make_unique<XboxOneController>(std::move(devicePtr));
                }
                else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 255, 255)))
                {
                    WriteToLog("Registering Xbox One adapter");
                    devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                    controllerPtr = std::make_unique<XboxOneAdapter>(std::move(devicePtr));
                }
            }
        }
        rc = eventWait(&ds3Event, 0);
        if (R_SUCCEEDED(rc))
        {
            WriteToLog("Dualshock 3 event went off");
            UsbHsInterface interfaces[4];
            s32 total_entries;

            if (R_SUCCEEDED(QueryVendorProduct(interfaces, sizeof(interfaces), &total_entries, VENDOR_SONY, PRODUCT_DUALSHOCK3)))
            {
                WriteToLog("Registering DS3 controller");
                devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                controllerPtr = std::make_unique<Dualshock3Controller>(std::move(devicePtr));
            }
        }
        rc = eventWait(&ds4Event, 0);
        if (R_SUCCEEDED(rc))
        {
            WriteToLog("Dualshock 4 event went off");
            UsbHsInterface interfaces[4];
            s32 total_entries;

            if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_HID, 0, 0)))
            {
                WriteToLog("Registering DS4 controller");
                devicePtr = std::make_unique<SwitchUSBDevice>(interfaces, total_entries);
                controllerPtr = std::make_unique<Dualshock4Controller>(std::move(devicePtr));
            }
        }
        CallInitHandler();

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
    usbHsDestroyInterfaceAvailableEvent(&ds3Event, 0);
    usbHsDestroyInterfaceAvailableEvent(&catchAllEvent, 1);
    usbHsDestroyInterfaceAvailableEvent(&ds4Event, 2);

    //controllerInterfaces.clear();
    return rc;
}