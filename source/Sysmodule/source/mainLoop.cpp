#include <switch.h>
#include "mainLoop.h"
#include <variant>
#include "log.h"

#include "SwitchUSBDevice.h"
#include "ControllerHelpers.h"
#include "SwitchHDLHandler.h"
#include "SwitchAbstractedPadHandler.h"
#include "configFile.h"
#include "SwitchThread.h"

#define APP_VERSION "0.6.0"

#define DS3EVENT_INDEX 0
#define DS4EVENT_INDEX 1
#define ALLEVENT_INDEX 2

#ifdef __APPLET__
#define APPLET_STACKSIZE 0x1'000
#else
#define APPLET_STACKSIZE 0x0
#endif

static const bool useAbstractedPad = hosversionBetween(5, 7);
std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> controllerInterfaces;
std::unique_ptr<IController> controllerPtr;
UsbHsInterface interfaces[16];
s32 total_entries;
UsbHsInterfaceFilter g_filter;

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
            WriteToLog("Error creating interface with error 0x%x", rc);
            return rc;
        }
    }
    return 1;
}

Result CreateDualshck3AvailableEvent(Event &out)
{
    g_filter = {};
    g_filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
    g_filter.idVendor = VENDOR_SONY;
    g_filter.idProduct = PRODUCT_DUALSHOCK3;
    Result rc = usbHsCreateInterfaceAvailableEvent(&out, true, DS3EVENT_INDEX, &g_filter);
    return rc;
}

Result CreateDualshock4AvailableEvent(Event &out)
{
    g_filter = {};
    g_filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
    g_filter.idVendor = VENDOR_SONY;
    g_filter.idProduct = _globalConfig.dualshock4_productID;
    Result rc = usbHsCreateInterfaceAvailableEvent(&out, true, DS4EVENT_INDEX, &g_filter);
    return rc;
}

Result CreateAllAvailableEvent(Event &out)
{
    g_filter = {};
    g_filter.Flags = UsbHsInterfaceFilterFlags_bcdDevice_Min;
    g_filter.bcdDevice_Min = 0;
    Result rc = usbHsCreateInterfaceAvailableEvent(&out, true, ALLEVENT_INDEX, &g_filter);
    return rc;
}

Result QueryInterfaces(UsbHsInterface *interfaces, size_t interfaces_size, s32 *total_entries, u8 infclass, u8 infsubclass, u8 infprotocol)
{
    g_filter = {};
    g_filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass | UsbHsInterfaceFilterFlags_bInterfaceProtocol;
    g_filter.bInterfaceClass = infclass;
    g_filter.bInterfaceSubClass = infsubclass;
    g_filter.bInterfaceProtocol = infprotocol;
    Result rc = usbHsQueryAvailableInterfaces(&g_filter, interfaces, interfaces_size, total_entries);
    if (R_SUCCEEDED(rc) && *total_entries != 0)
        return 0;
    else
        return 1;
}

Result QueryVendorProduct(UsbHsInterface *interfaces, size_t interfaces_size, s32 *total_entries, uint16_t vendor_id, uint16_t product_id)
{
    g_filter = {};
    g_filter.Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct;
    g_filter.idVendor = vendor_id;
    g_filter.idProduct = product_id;
    Result rc = usbHsQueryAvailableInterfaces(&g_filter, interfaces, interfaces_size, total_entries);
    if (R_SUCCEEDED(rc) && *total_entries != 0)
        return 0;
    else
        return 1;
}

Event catchAllEvent;
Event ds3Event;
Event ds4Event;

Result inline OpenEvents()
{
    Result rc = CreateAllAvailableEvent(catchAllEvent);
    if (R_FAILED(rc))
        return 3;
    rc = CreateDualshck3AvailableEvent(ds3Event);
    if (R_FAILED(rc))
        return 1;
    rc = CreateDualshock4AvailableEvent(ds4Event);
    if (R_FAILED(rc))
        return 2;

    return 0;
}

void inline CloseEvents()
{
    usbHsDestroyInterfaceAvailableEvent(&ds3Event, DS3EVENT_INDEX);
    usbHsDestroyInterfaceAvailableEvent(&ds4Event, DS4EVENT_INDEX);
    usbHsDestroyInterfaceAvailableEvent(&catchAllEvent, ALLEVENT_INDEX);
}

struct PSCLoopBuffer
{
    PscPmModule &pscModule;
    bool &shouldSleep;
};

void pscLoop(void *buffer)
{
    PscPmState pscState;
    u32 out_flags;

    Result rc = waitSingle(waiterForEvent(&static_cast<PSCLoopBuffer *>(buffer)->pscModule.event), U64_MAX);
    if (R_SUCCEEDED(rc))
    {
        rc = pscPmModuleGetRequest(&static_cast<PSCLoopBuffer *>(buffer)->pscModule, &pscState, &out_flags);
        if (R_SUCCEEDED(rc))
        {
            switch (pscState)
            {
            case PscPmState_ReadyAwaken:
                OpenEvents();
                static_cast<PSCLoopBuffer *>(buffer)->shouldSleep = false;
                break;
            case PscPmState_ReadySleep:
            case PscPmState_ReadyShutdown:
                CloseEvents();
                static_cast<PSCLoopBuffer *>(buffer)->shouldSleep = true;
                //controllerInterfaces.clear();
                break;
            default:
                break;
            }
            pscPmModuleAcknowledge(&static_cast<PSCLoopBuffer *>(buffer)->pscModule, pscState);
        }
    }
}

void CheckForInterfaces()
{
    if (controllerInterfaces.size() >= 10)
        WriteToLog("But the controllers table reached its max size!");
    else
    {

        if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 93, 1)))
        {
            controllerPtr = std::make_unique<Xbox360Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
            WriteToLog("Registered Xbox 360 controller");
        }
        else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 93, 129)))
        {
            for (int i = 0; i != total_entries; ++i)
            {
                controllerPtr = std::make_unique<Xbox360WirelessController>(std::make_unique<SwitchUSBDevice>(interfaces + i, 1));
                CallInitHandler(controllerPtr);
            }
            WriteToLog("Registered Xbox 360 Wireless adapter");
        }
        else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, 0x58, 0x42, 0x00)))
        {
            controllerPtr = std::make_unique<XboxController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
            WriteToLog("Registered Xbox controller");
        }
        else if (R_SUCCEEDED(QueryInterfaces(interfaces, sizeof(interfaces), &total_entries, USB_CLASS_VENDOR_SPEC, 71, 208)))
        {
            controllerPtr = std::make_unique<XboxOneController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
            WriteToLog("Registered Xbox One controller");
        }
        else if (R_SUCCEEDED(QueryVendorProduct(interfaces, sizeof(interfaces), &total_entries, VENDOR_SONY, PRODUCT_DUALSHOCK3)))
        {
            controllerPtr = std::make_unique<Dualshock3Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
            WriteToLog("Registered DS3 controller");
        }
        else if (R_SUCCEEDED(QueryVendorProduct(interfaces, sizeof(interfaces), &total_entries, VENDOR_SONY, _globalConfig.dualshock4_productID)))
        {
            controllerPtr = std::make_unique<Dualshock4Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries));
            WriteToLog("Registered DS4 controller");
        }
        CallInitHandler(controllerPtr);
    }
}

void eventLoop(void *buffer)
{
    if (R_SUCCEEDED(eventWait(static_cast<Event *>(buffer), U64_MAX)))
    {
        CheckForInterfaces();
        WriteToLog("Catch-all event went off");
    }
}

Result errorLoop()
{
#ifdef __APPLET__
    WriteToLog("Press B to exit...");
    while (appletMainLoop())
    {
        hidScanInput();

        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_B)
            break;
        LockedUpdateConsole();
    }
#endif
    return 0;
}

Result mainLoop()
{
    WriteToLog("\n\nNew sysmodule session started on version " APP_VERSION);
    Result rc = 0;

    UTimer filecheckTimer;
    Waiter filecheckTimerWaiter = waiterForUTimer(&filecheckTimer);
    utimerCreate(&filecheckTimer, 1e+9L, TimerType_Repeating);
    utimerStart(&filecheckTimer);

    PscPmModule pscModule;
    const uint16_t dependencies[] = {PscPmModuleId_Usb};

    rc = pscmGetPmModule(&pscModule, static_cast<PscPmModuleId>(126), dependencies, sizeof(dependencies) / sizeof(uint16_t), true);
    WriteToLog("Get module result: 0x%x", rc);
    if (R_FAILED(rc))
        return errorLoop();

    bool shouldSleep = false;
    PSCLoopBuffer loopBuffer{pscModule, shouldSleep};

    SwitchThread pscThread = SwitchThread(pscLoop, &loopBuffer, 0x300, 0x3B);
    WriteToLog("Is psc thread running: %i", pscThread.IsRunning());

    CheckForFileChanges();
    LoadAllConfigs();

    rc = OpenEvents();
    if (R_FAILED(rc))
    {
        WriteToLog("Failed to open events: 0x%x", rc);
        return errorLoop();
    }
    controllerInterfaces.reserve(10);

    constexpr size_t eventThreadStack = 0x1'500 + APPLET_STACKSIZE;
    SwitchThread eventThread(&eventLoop, &catchAllEvent, eventThreadStack, 0x20);
    WriteToLog("Is event thread running: %i", eventThread.IsRunning());

    while (appletMainLoop())
    {
        if (!shouldSleep)
        {

#ifdef __APPLET__
            hidScanInput();
            u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

            for (int i = 0; i != 8; ++i)
            {
                u64 kHeld = hidKeysDown(static_cast<HidControllerID>(i));
                if (kHeld != 0)
                    WriteToLog("Player %i: %lu", i + 1, kHeld);
            }

            if (kDown & KEY_B)
                break;
#endif
            //On interface change event, check if any devices were removed, and erase them from memory appropriately
            rc = eventWait(usbHsGetInterfaceStateChangeEvent(), 0);
            if (R_SUCCEEDED(rc))
            {
                WriteToLog("Interface state was changed");
                eventClear(usbHsGetInterfaceStateChangeEvent());
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
                            WriteToLog("Erasing controller! %i", (*it)->GetController()->GetType());
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
                    usbHsDestroyInterfaceAvailableEvent(&ds4Event, DS4EVENT_INDEX);
                    CreateDualshock4AvailableEvent(ds4Event);
                }
            }
        }
        else if (controllerInterfaces.size() != 0)
        {
            WriteToLog("Clearing interfaces before going to sleep");
            controllerInterfaces.clear();
        }
#ifdef __APPLET__
        LockedUpdateConsole();
#else
        svcSleepThread(1e+7L);
#endif
    }

    WriteToLog("Closing PSC module");
    pscPmModuleFinalize(&pscModule);
    pscPmModuleClose(&pscModule);
    eventClose(&pscModule.event);
    pscThread.Exit();

    WriteToLog("Destroying events");
    CloseEvents();
    eventThread.Exit();

    WriteToLog("Clearing interfaces");
    controllerInterfaces.clear();
    return rc;
}