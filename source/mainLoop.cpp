#include <switch.h>
#include "mainLoop.h"
#include <variant>
#include "log.h"

#include "SwitchUSBDevice.h"
#include "ControllerHelpers.h"
#include "SwitchHDLHandler.h"
#include "SwitchAbstractedPadHandler.h"
#include "configFile.h"

#define APP_VERSION "0.5.1"

static const bool useAbstractedPad = hosversionBetween(5, 7);
std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> controllerInterfaces;

static GlobalConfig _globalConfig{};

void LoadGlobalConfig(const GlobalConfig *config)
{
    _globalConfig = *config;
}

Result CallInitHandler(std::unique_ptr<IController> &controllerPtr)
{
    if (controllerPtr)
    {
        std::unique_ptr<SwitchVirtualGamepadHandler> switchHandler;
        if (useAbstractedPad)
            switchHandler = std::make_unique<SwitchAbstractedPadHandler>(std::move(controllerPtr));
        else
            switchHandler = std::make_unique<SwitchHDLHandler>(std::move(controllerPtr));

        Result rc = switchHandler->Initialize();
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

Result CreateDualshck3AvailableEvent(Event &out)
{
    UsbHsInterfaceFilter filter;
    filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
    filter.idVendor = VENDOR_SONY;
    filter.idProduct = PRODUCT_DUALSHOCK3;
    Result rc = usbHsCreateInterfaceAvailableEvent(&out, true, 0, &filter);
    if (R_FAILED(rc))
        WriteToLog("Failed to open event for Dualshock 3");
    else
        WriteToLog("Successfully created event for Dualshock 3");
    return rc;
}

Result CreateDualshock4AvailableEvent(Event &out)
{
    UsbHsInterfaceFilter filter;
    filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
    filter.idVendor = VENDOR_SONY;
    filter.idProduct = _globalConfig.dualshock4_productID;
    Result rc = usbHsCreateInterfaceAvailableEvent(&out, true, 1, &filter);
    if (R_FAILED(rc))
        WriteToLog("Failed to open event for Dualshock 4 0x", std::hex, _globalConfig.dualshock4_productID);
    else
        WriteToLog("Successfully created event for Dualshock 4 0x", std::hex, _globalConfig.dualshock4_productID);
    return rc;
}

Result CreateAllAvailableEvent(Event &out)
{
    UsbHsInterfaceFilter filter;
    filter.Flags = UsbHsInterfaceFilterFlags_bcdDevice_Min;
    filter.bcdDevice_Min = 0;
    Result rc = usbHsCreateInterfaceAvailableEvent(&out, true, 2, &filter);
    if (R_FAILED(rc))
        WriteToLog("Failed to open catch-all event");
    else
        WriteToLog("Successfully created catch-all event");
    return rc;
}

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

Result mainLoop()
{
    WriteToLog("\n\nNew sysmodule session started on version " APP_VERSION);
    Result rc = 0;

    std::unique_ptr<IController> controllerPtr;

    UTimer filecheckTimer;
    Waiter filecheckTimerWaiter = waiterForUTimer(&filecheckTimer);
    utimerCreate(&filecheckTimer, 1e+9L, TimerType_Repeating);
    utimerStart(&filecheckTimer);

    CheckForFileChanges();
    LoadAllConfigs();

    Event catchAllEvent;
    Event ds3Event;
    Event ds4Event;

    CreateDualshck3AvailableEvent(ds3Event);
    CreateAllAvailableEvent(catchAllEvent);
    CreateDualshock4AvailableEvent(ds4Event);

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
                    controllerPtr = std::make_unique<Xbox360Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
                }
                else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 93, 129)))
                {
                    WriteToLog("Registering Xbox 360 Wireless adapter");
                    for (int i = 0; i != total_entries; ++i)
                    {
                        controllerPtr = std::make_unique<Xbox360WirelessController>(std::make_unique<SwitchUSBDevice>(interfaces + i, 1));
                        CallInitHandler(controllerPtr);
                    }
                }
                else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, 0x58, 0x42, 0x00)))
                {
                    WriteToLog("Registering Xbox One controller");
                    controllerPtr = std::make_unique<XboxController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
                }
                else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 71, 208)))
                {
                    WriteToLog("Registering Xbox One controller");
                    controllerPtr = std::make_unique<XboxOneController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
                }
                /*
                else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 255, 255)))
                {
                    WriteToLog("Registering Xbox One adapter");
                    controllerPtr = std::make_unique<XboxOneAdapter>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
                }
                */
            }
        }
        rc = eventWait(&ds3Event, 0);
        if (R_SUCCEEDED(rc))
        {
            WriteToLog("Dualshock 3 event went off");
            UsbHsInterface interfaces[4];
            s32 total_entries;

            if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_HID, 0, 0)))
            {
                WriteToLog("Registering DS3 controller");
                controllerPtr = std::make_unique<Dualshock3Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
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
                controllerPtr = std::make_unique<Dualshock4Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
            }
        }
        CallInitHandler(controllerPtr);

        //On interface change event, check if any devices were removed, and erase them from memory appropriately
        rc = eventWait(usbHsGetInterfaceStateChangeEvent(), 0);
        if (R_SUCCEEDED(rc))
        {
            WriteToLog("Interface state was changed");
            eventClear(usbHsGetInterfaceStateChangeEvent());

            UsbHsInterface interfaces[16];
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

        //Checks every 1 second for any changes in config files, then reloads them
        rc = waitSingle(filecheckTimerWaiter, 0);
        if (R_SUCCEEDED(rc))
        {
            if (CheckForFileChanges())
            {
                WriteToLog("File check succeeded! Loading configs...");
                LoadAllConfigs();
                usbHsDestroyInterfaceAvailableEvent(&ds4Event, 2);
                CreateDualshock4AvailableEvent(ds4Event);
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

    controllerInterfaces.clear();
    return rc;
}