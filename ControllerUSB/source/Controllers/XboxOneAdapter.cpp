#include "Controllers/XboxOneAdapter.h"
#include "Controllers/XboxOneAdapter/Firmware.h"
#include <cmath>
#include "../../source/log.h"
#include <fstream>
#include "cstring"

static ControllerConfig _xboxoneadapterConfig{};
static char firmwarePath[100];

XboxOneAdapter::XboxOneAdapter(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

XboxOneAdapter::~XboxOneAdapter()
{
    Exit();
}

Result XboxOneAdapter::Initialize()
{
    Result rc;

    rc = OpenInterfaces();
    if (R_FAILED(rc))
        return rc;

    rc = SendInitBytes();
    if (R_FAILED(rc))
        return rc;
    return rc;
}
void XboxOneAdapter::Exit()
{
    CloseInterfaces();
}

Result XboxOneAdapter::OpenInterfaces()
{
    Result rc;
    rc = m_device->Open();
    if (R_FAILED(rc))
        return rc;

    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (R_FAILED(rc))
            return rc;

        if (interface->GetDescriptor()->bInterfaceProtocol != 255)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

        m_interface = interface.get();

        if (!m_inPipePacket)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, 3);
            if (inEndpoint)
            {
                rc = inEndpoint->Open();
                if (R_FAILED(rc))
                    return 3333;

                m_inPipePacket = inEndpoint;
            }
        }

        if (!m_inPipe)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, 4);
            if (inEndpoint)
            {
                rc = inEndpoint->Open();
                if (R_FAILED(rc))
                    return 4444;

                m_inPipe = inEndpoint;
            }
        }

        if (!m_outPipe)
        {
            IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, 3);
            if (outEndpoint)
            {
                rc = outEndpoint->Open();
                if (R_FAILED(rc))
                    return 5555;

                m_outPipe = outEndpoint;
            }
        }
    }

    if (!m_inPipe || !m_outPipe || !m_inPipePacket)
        return 69;

    return rc;
}

void XboxOneAdapter::CloseInterfaces()
{
    //m_device->Reset();
    m_device->Close();
}

Result XboxOneAdapter::SendInitBytes()
{
    Result rc;
    DmaConfig config = {};

    config.rxBulkEnabled = 1;
    config.txBulkEnabled = 1;

    ControlWrite(m_interface, MT_USB_U3DMA_CFG, config.value, MT_VEND_WRITE_CFG);
    ControlWrite(m_interface, MT_FCE_PSE_CTRL, 0x01);
    ControlWrite(m_interface, MT_TX_CPU_FROM_FCE_BASE_PTR, 0x400230);
    ControlWrite(m_interface, MT_TX_CPU_FROM_FCE_MAX_COUNT, 0x01);
    ControlWrite(m_interface, MT_TX_CPU_FROM_FCE_CPU_DESC_IDX, 0x01);
    ControlWrite(m_interface, MT_FCE_PDMA_GLOBAL_CONF, 0x44);
    ControlWrite(m_interface, MT_FCE_SKIP_FS, 0x03);

    WriteToLog("firmware path: %s", firmwarePath);
    if (!firmwarePath || *firmwarePath == '\0')
    {
        WriteToLog("But the string is empty!");
        return 256;
    }

    std::ifstream fs(firmwarePath, std::ios::binary);
    if (fs.fail())
        return 235;

    WriteToLog("Opening file...");

    std::vector<uint8_t> firmware(std::istreambuf_iterator<char>(fs), {});

    WriteToLog("File opened!");

    fs.close();

    WriteToLog("writing %lu bytes...", firmware.size());

    FwHeader *header = reinterpret_cast<FwHeader *>(firmware.data());

    uint8_t *ilmStart = reinterpret_cast<uint8_t *>(header) + sizeof(FwHeader);
    uint8_t *dlmStart = ilmStart + header->ilmLength;
    uint8_t *dlmEnd = dlmStart + header->dlmLength;

    WriteToLog("Writing 1st part");

    rc = LoadFirmwarePart(MT_MCU_ILM_OFFSET, ilmStart, dlmStart);
    if (R_FAILED(rc))
        return rc;

    WriteToLog("Writing 2nd part");
    rc = LoadFirmwarePart(MT_MCU_DLM_OFFSET, dlmStart, dlmEnd);
    if (R_FAILED(rc))
        return rc;

    WriteToLog("Wrote");

    return 0;
}

Result XboxOneAdapter::LoadFirmwarePart(uint32_t offset, uint8_t *start, uint8_t *end)
{
    // Send firmware in chunks
    Result rc = -1;
    for (uint8_t *chunk = start; chunk < end; chunk += MT_FW_CHUNK_SIZE)
    {
        uint32_t address = (uint32_t)(offset + chunk - start);
        uint32_t remaining = (uint32_t)(end - chunk);
        uint16_t length = remaining > MT_FW_CHUNK_SIZE ? MT_FW_CHUNK_SIZE : remaining;

        rc = ControlWrite(m_interface, MT_FCE_DMA_ADDR, address, MT_VEND_WRITE_CFG);
        if (R_FAILED(rc))
            return rc;

        rc = ControlWrite(m_interface, MT_FCE_DMA_LEN, length << 16, MT_VEND_WRITE_CFG);
        if (R_FAILED(rc))
            return rc;

        uint8_t data[length + 8]{0x00, 0x38, 0x00, 0x10};

        for (int i = 0; i != length; ++i)
        {
            data[i + 4] = chunk[i];
        }

        rc = m_outPipe->Write(data, sizeof(data));
        if (R_FAILED(rc))
            return rc;
    }
    return rc;
}

Result XboxOneAdapter::ControlWrite(IUSBInterface *interface, uint16_t address, uint32_t value, VendorRequest request)
{
    Result rc;
    if (request == MT_VEND_DEV_MODE)
    {
        rc = interface->ControlTransfer(0x40, request, address, 0, 0, static_cast<const void *>(nullptr));
    }
    else
    {
        rc = interface->ControlTransfer(0x40, request, address, 0, sizeof(uint32_t), &value);
    }
    return rc;
}

void XboxOneAdapter::LoadConfig(const ControllerConfig *config, const char *path)
{
    _xboxoneadapterConfig = *config;
    strcpy(firmwarePath, path);
}

ControllerConfig *XboxOneAdapter::GetConfig()
{
    return &_xboxoneadapterConfig;
}