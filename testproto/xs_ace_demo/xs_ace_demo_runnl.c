#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
char buf[1024]="";

int main(int argc, char **argv)
{
int i = 0;
for (; i < argc; i++) {
    printf("%s\n", *(argv + i));
    fflush(stdout);
}
int cont =1;
while (cont) {
memset(buf, 0, sizeof buf);
if(fgets(buf, sizeof buf, stdin) == NULL)
    exit(0);
char *p = strchr(buf,'\n');
if(p)
*p='\0';
//if (!strcmp(buf, "exit"))
  //  break;
strcat(buf, ": response");

if (printf("%d %s\n",strlen(buf), buf) < 0) {
    printf("exit\n");
    exit(0);
}
//printf("%s\n", buf);
fflush(stdout);
}
}
