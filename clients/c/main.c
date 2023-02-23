#include <stdlib.h>
#include <stdio.h>
#include "./net/client.h"

int main(){
    tfprotocol.header_size = 4;
    int result = tf_connect("127.0.0.1", 10345);
    shutdown(tfprotocol.socket, 1);
    return 0;
}