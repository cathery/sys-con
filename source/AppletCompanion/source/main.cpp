#include "switch.h"
#include <stdio.h>

int main()
{
	consoleInit(NULL);
	
	printf("Hello\n");
	
	while(appletMainLoop())
	{
		hidScanInput();
		u64 kDown = 0;
		for (u8 controller = 0; controller < 10; controller++)
			kDown |= hidKeysDown(static_cast<HidControllerID>(controller));
		
		if (kDown & KEY_PLUS || kDown & KEY_B)
			break;
		consoleUpdate(NULL);
	}

	consoleExit(NULL);
	return 0;
}