#pragma once
#include "switch.h"

//This file exists for the sole purpose of fixing existing libnx bugs while the new release isn't out yet

//regular usbHsEpClose incorrectly accesses more memory than it should, causing a crash
void usbHsEpCloseFixed(UsbHsClientEpSession *s);
