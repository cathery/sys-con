#include "libnxFix.h"
#include <cstring>
#include <stdio.h>
#include "malloc.h"

static Result _usbHsCmdNoIO(Service *s, u64 cmd_id)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct Packet
    {
        u64 magic;
        u64 cmd_id;
    } * raw;

    raw = static_cast<Packet *>(serviceIpcPrepareHeader(s, &c, sizeof(*raw)));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc))
    {
        IpcParsedCommand r;
        struct Reponse
        {
            u64 magic;
            u64 result;
        } * resp;

        serviceIpcParse(s, &r, sizeof(*resp));
        resp = static_cast<Reponse *>(r.Raw);

        rc = resp->result;
    }

    return rc;
}
void usbHsEpCloseFixed(UsbHsClientEpSession *s)
{
    if (!serviceIsActive(&s->s))
        return;

    _usbHsCmdNoIO(&s->s, hosversionAtLeast(2, 0, 0) ? 1 : 3); //Close

    serviceClose(&s->s);
    eventClose(&s->eventXfer);
    memset(s, 0, sizeof(UsbHsClientEpSession));
}