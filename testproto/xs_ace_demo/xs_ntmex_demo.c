#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void xs_ntmex(int64_t (*r_io)(char *buf, int64_t len),
    int64_t (*w_io)(char *buf, int64_t len), 
    int64_t (*rex_io)(char *buf, int64_t len),
    int64_t (*wex_io)(char *buf, int64_t len), 
    int (*jd)(const char *src,char *dst))
{

char buf[10000];
memset(buf, 0, sizeof buf);
r_io(buf, sizeof buf);
printf("%s\n", buf);

strcpy(buf, "go the hell!");
w_io(buf, strlen(buf));

}
