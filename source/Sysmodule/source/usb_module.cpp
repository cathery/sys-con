#include "switch.h"
#include "usb_module.h"
#include "controller_handler.h"
#include "config_handler.h"


#include "SwitchUSBDevice.h"
#include "ControllerHelpers.h"
#include "log.h"

namespace syscon::usb
{
    namespace
    {
        constexpr u8 CatchAllEventIndex = 2;
        constexpr u8 Dualshock3EventIndex = 0;
        constexpr u8 Dualshock4EventIndex = 1;


        constexpr size_t MaxUsbHsInterfacesSize = 16;

        void UsbEventThreadFunc(void *arg);
        void UsbInterfaceChangeThreadFunc(void *arg);

        ams::os::StaticThread<0x2'000> g_usb_event_thread(&UsbEventThreadFunc, nullptr, 0x20);
        ams::os::StaticThread<0x2'000> g_usb_interface_change_thread(&UsbInterfaceChangeThreadFunc, nullptr, 0x21);

        bool is_usb_event_thread_running = false;
        bool is_usb_interface_change_thread_running = false;

        Event g_usbCatchAllEvent;
        Event g_usbDualshock3Event;
        Event g_usbDualshock4Event;
        UsbHsInterface interfaces[MaxUsbHsInterfacesSize];

        s32 QueryInterfaces(u8 iclass, u8 isubclass, u8 iprotocol);
        s32 QueryVendorProduct(uint16_t vendor_id, uint16_t product_id);

        void UsbEventThreadFunc(void *arg)
        {
            WriteToLog("Starting USB Event Thread!");
            do {
                if (R_SUCCEEDED(eventWait(&g_usbCatchAllEvent, U64_MAX)))
                {
                    WriteToLog("Event got caught!");
                    if (handler::IsAtControllerLimit())
                        continue;
                    WriteToLog("Querying interfaces now");
                    s32 total_entries;
                    if ((total_entries = QueryInterfaces(USB_CLASS_VENDOR_SPEC, 93, 1)) != 0)
                        handler::Insert(std::make_unique<Xbox360Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries)));

                    else if ((total_entries = QueryInterfaces(USB_CLASS_VENDOR_SPEC, 93, 129)) != 0)
                        for (int i = 0; i != total_entries; ++i)
                            handler::Insert(std::make_unique<Xbox360WirelessController>(std::make_unique<SwitchUSBDevice>(interfaces + i, 1)));

                    else if ((total_entries = QueryInterfaces(0x58, 0x42, 0x00)) != 0)
                        handler::Insert(std::make_unique<XboxController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries)));
                    
                    else if ((total_entries = QueryInterfaces(USB_CLASS_VENDOR_SPEC, 71, 208)) != 0)
                        handler::Insert(std::make_unique<XboxOneController>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries)));

                    else if ((total_entries = QueryVendorProduct(VENDOR_SONY, PRODUCT_DUALSHOCK3)) != 0)
                        handler::Insert(std::make_unique<Dualshock3Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries)));

                    else if ((total_entries = QueryVendorProduct(VENDOR_SONY, PRODUCT_DUALSHOCK4_2X)) != 0)
                        handler::Insert(std::make_unique<Dualshock4Controller>(std::make_unique<SwitchUSBDevice>(interfaces, total_entries)));
                }
            } while (is_usb_event_thread_running);
        }

        void UsbInterfaceChangeThreadFunc(void *arg)
        {
            WriteToLog("Starting USB Interface Change Thread!");
            do {
                if (R_SUCCEEDED(eventWait(usbHsGetInterfaceStateChangeEvent(), UINT64_MAX)))
                {
                    s32 total_entries;
                    WriteToLog("Interface state was changed");
                    eventClear(usbHsGetInterfaceStateChangeEvent());
                    if (R_SUCCEEDED(usbHsQueryAcquiredInterfaces(interfaces, sizeof(interfaces), &total_entries)))
                    {
                        for (auto it = handler::Get().begin(); it != handler::Get().end(); ++it)
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
                                //WriteToLog("Erasing controller! %i", (*it)->GetController()->GetType());
                                handler::Get().erase(it--);
                                WriteToLog("Controller erased!");
                            }
                        }
                    }
                }
            } while (is_usb_interface_change_thread_running);
        }

        s32 QueryInterfaces(u8 iclass, u8 isubclass, u8 iprotocol)
        {
            UsbHsInterfaceFilter filter {
                .Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass | UsbHsInterfaceFilterFlags_bInterfaceProtocol,
                .bInterfaceClass = iclass,
                .bInterfaceSubClass = isubclass,
                .bInterfaceProtocol = iprotocol,
            };
            s32 out_entries = 0;
            usbHsQueryAvailableInterfaces(&filter, interfaces, sizeof(interfaces), &out_entries);
            return out_entries;
        }

        s32 QueryVendorProduct(uint16_t vendor_id, uint16_t product_id)
        {
            UsbHsInterfaceFilter filter {
                .Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct,
                .idVendor = vendor_id,
                .idProduct = product_id,
            };
            s32 out_entries = 0;
            usbHsQueryAvailableInterfaces(&filter, interfaces, sizeof(interfaces), &out_entries);
            return out_entries;
        }

        Result CreateCatchAllAvailableEvent()
        {
            constexpr UsbHsInterfaceFilter filter {
                .Flags = UsbHsInterfaceFilterFlags_bcdDevice_Min,
                .bcdDevice_Min = 0,
            };
            return usbHsCreateInterfaceAvailableEvent(&g_usbCatchAllEvent, true, CatchAllEventIndex, &filter);
        }

        Result CreateDualshock3AvailableEvent()
        {
            constexpr UsbHsInterfaceFilter filter {
                .Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct,
                .idVendor = VENDOR_SONY,
                .idProduct = PRODUCT_DUALSHOCK3,
            };
            return usbHsCreateInterfaceAvailableEvent(&g_usbDualshock3Event, true, Dualshock3EventIndex, &filter);
        }

        Result CreateDualshock4AvailableEvent()
        {
            const UsbHsInterfaceFilter filter{
                .Flags = UsbHsInterfaceFilterFlags_idVendor | UsbHsInterfaceFilterFlags_idProduct,
                .idVendor = VENDOR_SONY,
                .idProduct = config::globalConfig.dualshock4_productID,
            };
            return usbHsCreateInterfaceAvailableEvent(&g_usbDualshock4Event, true, Dualshock4EventIndex, &filter);
        }
    }


    void Initialize()
    {
        R_ASSERT(Enable());
    }

    void Exit()
    {
        Disable();
    }

    Result Enable()
    {
        CreateUsbEvents();

        is_usb_event_thread_running = true;
        R_TRY(g_usb_event_thread.Start().GetValue());
        is_usb_interface_change_thread_running = true;
        R_TRY(g_usb_interface_change_thread.Start().GetValue());
        return 0;
    }

    void Disable()
    {
        DestroyUsbEvents();

        is_usb_event_thread_running = false;
        is_usb_interface_change_thread_running = false;

        //TODO: test this without the cancel
        g_usb_event_thread.CancelSynchronization();
        g_usb_interface_change_thread.CancelSynchronization();

        g_usb_event_thread.Join();
        g_usb_interface_change_thread.Join();

        handler::Reset();
    }

    Result CreateUsbEvents()
    {
        R_TRY(CreateCatchAllAvailableEvent());
        R_TRY(CreateDualshock3AvailableEvent());
        R_TRY(CreateDualshock4AvailableEvent());
        return 0;
    }

    void DestroyUsbEvents()
    {
        usbHsDestroyInterfaceAvailableEvent(&g_usbCatchAllEvent, CatchAllEventIndex);
        usbHsDestroyInterfaceAvailableEvent(&g_usbDualshock3Event, Dualshock3EventIndex);
        usbHsDestroyInterfaceAvailableEvent(&g_usbDualshock4Event, Dualshock4EventIndex);
    }

    Result ReloadDualshock4Event()
    {
        usbHsDestroyInterfaceAvailableEvent(&g_usbDualshock4Event, Dualshock4EventIndex);
        return CreateDualshock4AvailableEvent();
    }
}