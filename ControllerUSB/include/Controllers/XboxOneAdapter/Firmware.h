#pragma once
#include <stdint.h>

#define MT_FW_RESOURCE "Firmware.bin"
#define MT_MCU_ILM_OFFSET 0x80000
#define MT_MCU_DLM_OFFSET 0x100000 + 0x10800
#define MT_FW_CHUNK_SIZE 0x3800
#define MT_DMA_COMPLETE 0xc0000000
#define MT_FW_LOAD_IVB 0x12
