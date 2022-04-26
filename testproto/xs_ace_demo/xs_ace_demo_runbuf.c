#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <limits.h>
char buf[PIPE_BUF];

int main(int argc, char **argv)
{

while(1) {
memset(buf, 0, sizeof buf);
read(0, buf, sizeof buf);
write(1, buf, strlen(buf)); 
}
}
