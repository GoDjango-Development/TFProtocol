#include <stdlib.h>
#include <stdio.h>
#include "./net/client.h"

int main(){
    int sock = tf_connect("localhost", 10345);
    shutdown(sock, 1);
    return 0;
}