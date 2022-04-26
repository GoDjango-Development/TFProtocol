#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <errno.h>

#define swapbo32(value) value = (value >> 24 & 0xff) | (value >> 16 & 0xff) << \
    8 | (value >> 8 & 0xff) << 16 | (value & 0xff) << 24;

char deckey[1024]="";
int declen=0;
int encrypt =1;
void decrypt(char *data, int len)
{
 int c = 0;
        int keyc = 0;
     
        for (; c < len; c++) {
            if (keyc == declen)
                keyc = 0;
            *(data + c) ^= deckey[keyc++];
            
        }
}


int rds_real(int sock, char *buf, int len) 
{
    int rb = 0;
    int readed = 0;
    //int left = len;
    while (len > 0) {
        do 
            rb = read(sock, buf + readed, len);
        while (rb == -1 && errno == EINTR);
        if (rb == 0 || rb == -1)
            return -1;
        len -= rb;
        readed += rb;
    }
    if (encrypt)
        decrypt(buf, readed);
    return readed;
}

int wrs_real(int sock, char *buf, int len)
{
    if (encrypt)
        decrypt(buf, len);   
    int wb = 0;
    int writed = 0;
    //int left = len;
    while (len > 0) {
        do
            wb = write(sock, buf + writed, len);
        while (wb == -1 && errno == EINTR);
        if (wb == -1)
            return -1;
        len -= wb;
        writed += wb;
    }
    return writed;
}


int main(int argc, char **argv)
{
declen = atoi(*(argv + 2))/2;

int i =0;
int hex;
for (; i< declen;i++) {
  sscanf(*(argv + 1) + 2*i, "%02x", &hex);
  deckey[i]=hex;
}

char buf[10000]="";
int32_t hdr;
strcpy(buf,"OK");
hdr = strlen(buf);
int32_t len = hdr;
swapbo32(hdr);
wrs_real(1, &hdr, sizeof hdr);
wrs_real(1, buf, len);
int cont = 1;

while(cont) {
memset(buf, 0,sizeof buf);
rds_real(0, &hdr, sizeof hdr);
swapbo32(hdr);

rds_real(0, buf,hdr);

if (!strcmp(buf, "quit"))
cont = 0;

hdr = strlen(buf);
int32_t len = hdr;
swapbo32(hdr);
wrs_real(1, &hdr, sizeof hdr);
wrs_real(1, buf, len);
}
exit(0);
}
