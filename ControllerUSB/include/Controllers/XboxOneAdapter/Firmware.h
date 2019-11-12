#pragma once
#include <stdint.h>

#define MT_FW_RESOURCE "Firmware.bin"
#define MT_MCU_ILM_OFFSET 0x80000
#define MT_MCU_DLM_OFFSET 0x100000 + 0x10800
#define MT_FW_CHUNK_SIZE 0x3800
#define MT_DMA_COMPLETE 0xc0000000
#define MT_FW_LOAD_IVB 0x12

#define MT_FCE_DMA_ADDR 0x0230
#define MT_FCE_DMA_LEN 0x0234
#define MT_USB_DMA_CFG 0x0238

#define MT_USB_U3DMA_CFG 0x9018
#define MT_FCE_PSE_CTRL 0x0800
#define MT_TX_CPU_FROM_FCE_BASE_PTR 0x09a0
#define MT_TX_CPU_FROM_FCE_MAX_COUNT 0x09a4
#define MT_TX_CPU_FROM_FCE_CPU_DESC_IDX 0x09a8
#define MT_FCE_PDMA_GLOBAL_CONF 0x09c4
#define MT_FCE_SKIP_FS 0x0a6c

struct FwHeader
{
    uint32_t ilmLength;
    uint32_t dlmLength;
    uint16_t buildVersion;
    uint16_t firmwareVersion;
    uint32_t padding;
    char buildTime[16];
} __attribute__((packed));

union DmaConfig {
    struct
    {
        uint32_t rxBulkAggTimeout : 8;
        uint32_t rxBulkAggLimit : 8;
        uint32_t udmaTxWlDrop : 1;
        uint32_t wakeupEnabled : 1;
        uint32_t rxDropOrPad : 1;
        uint32_t txClear : 1;
        uint32_t txopHalt : 1;
        uint32_t rxBulkAggEnabled : 1;
        uint32_t rxBulkEnabled : 1;
        uint32_t txBulkEnabled : 1;
        uint32_t epOutValid : 6;
        uint32_t rxBusy : 1;
        uint32_t txBusy : 1;
    } __attribute__((packed));
    uint32_t value;
};
