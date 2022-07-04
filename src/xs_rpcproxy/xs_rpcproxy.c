#include <xs_rpcproxy/xs_rpcproxy.h>
#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <tfproto.h>

int64_t hdr;

static void runrpc(const char *bin);

void xs_rpcproxy(void)
{
    while (1) {
        if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        if (hdr == -1)
            break;
        if (!isbigendian())
            swapbo64(hdr);
        char bin[PATH_MAX]="";
        if (hdr > sizeof bin)
            hdr = sizeof bin;
        if ((hdr = readbuf_ex(bin, hdr)) == -1) {
            endcomm();
            return;
        }
        if (hdr == sizeof bin)
            hdr--;
        bin[hdr] = '\0';
    }
}

static void runrpc(const char *bin)
{
    
}
