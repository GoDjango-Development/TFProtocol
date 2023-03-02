#include <stdlib.h>
#include <stdio.h>
#include "./net/client.c"

const int initial_garbage_size = sizeof(char*)*5;
int free_garbage();

int main(){
    tfprotocol.header_size = 4;
    //tfprotocol.garbage = malloc(initial_garbage_size);
    int result = tf_connect("127.0.0.1", 10345);
    shutdown(tfprotocol.socket, 1);
    //free_garbage();
    return 0;
}
int free_garbage(){
    for(int i=0; i < initial_garbage_size; i+=sizeof(char*)){
        printf("Freeing memory 1\n");
        free(tfprotocol.garbage);
        tfprotocol.garbage += sizeof(char*);
        printf("Freeing memory\n");
    }
}