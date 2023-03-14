#include <xs_rpcproxy/xs_rpcproxy.h>
#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <init.h>
#include <tfproto.h>
#include <string.h>
#include <unistd.h>

#define EEXEC 127

int32_t hdr;
static int pdout[2];
static int pdin[2];
static pid_t pid;

static int runrpc(const char *hash);
static int getbin(const char *hash, char *bin);

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
            swapbo32(hdr);
        char hash[PATH_MAX];
        if (hdr > sizeof hash)
            hdr = sizeof hash;
        if ((hdr = readbuf_ex(hash, hdr)) == -1) {
            endcomm();
            return;
        }
        if (hdr == sizeof hash)
            hdr--;
        hash[hdr] = '\0';
        if (runrpc(hash) == -1) {
            hdr = -1;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        } else {
            hdr = 0;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            do {
                if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
                    endcomm();
                    return;
                }
                if (!isbigendian())
                    swapbo32(hdr);
                if (readbuf_ex(comm.buf, hdr) == -1) {
                    endcomm();
                    return;
                }
                writechunk(pdout[1], comm.buf, hdr);
            } while (hdr);
            close(pdout[1]);
            int64_t rb;
            while (rb = readchunk(pdin[0], comm.buf, sizeof comm.buf)) {
                if (rb == -1)
                    break;
                hdr = rb;
                if (!isbigendian())
                    swapbo32(hdr);
                if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                    endcomm();
                    return;
                }
                if (writebuf_ex(comm.buf, rb) == -1) {
                    endcomm();
                    return;
                }
            }
            close(pdin[0]);
            hdr = 0;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            hdr = sec_waitpid(pid);
            if (!isbigendian())
                swapbo32(hdr);
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        }
    }
}

static int runrpc(const char *hash)
{
    char bin[PATH_MAX];
    if (getbin(hash, bin) == -1) 
        return -1;
    int rs; 
    rs = pipe(pdin);
    rs += pipe(pdout);
    if (rs != 0) {
        close(pdin[0]);
        close(pdin[1]);
        close(pdout[0]);
        close(pdout[1]);
        return -1;
    }
    pid = fork();
    if (pid == -1) {
        close(pdin[0]);
        close(pdin[1]);
        close(pdout[0]);
        close(pdout[1]);
        return -1;
    }
    else if (!pid) {
        close(pdin[0]);
        close(pdout[1]);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        dup(pdout[0]);
        dup(pdin[1]);
        dup(pdin[1]);
        close(pdout[0]);
        close(pdin[1]);
        char *pt = strtok(bin, " ");
        if (pt) {
            pt = strtok(NULL, " ");
            execl(bin, bin, pt, NULL);
        } else
            execl(bin, bin, NULL);
        exit(EEXEC);
    }
    close(pdin[1]);
    close(pdout[0]);
    return 0;
}

static int getbin(const char *hash, char *bin)
{
    char ln[LINE_MAX];
    FILE *fp = fopen(tfproto.rpcproxy, "r");
    if (!fp) 
        return -1;
    while (fgets(ln, sizeof ln, fp)) {
        char *pt = strtok(ln, " ");
        if (pt && !strcmp(ln, hash)) {
            pt = strtok(NULL, "\n");
            strcpy(bin, pt);
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return -1;
}    
