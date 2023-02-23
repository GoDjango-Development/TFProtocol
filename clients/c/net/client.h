#ifndef TFCLIENT
#define TFCLIENT

/* Swap byte order of 32 bit integer: Big-to-Little or Little-to-Big endian. */
#define swapbo32(value) value = (value >> 24 & 0xFF) | (value >> 16 & 0xFF) << \
    8 | (value >> 8 & 0xFF) << 16 | (value & 0xFF) << 24;

/* Swap byte order of 64 bit integer: Big-to-Little or Little-to-Big endian. */
#define swapbo64(value) value = (value >> 56 & 0xFF) | (value >> 48 & 0xFF) << \
    8 | (value >> 40 & 0xFF) << 16 | (value >> 32 & 0xFF) << 24 | (value >> 24 \
    & 0xFF) << 32 | (value >> 16 & 0xFF) << 40 | (value >> 8 & 0xFF) << 48 | \
    (value & 0xFF) << 56


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include "client.c"

enum TF_CODES{
    OK, // It is usually 0 and means everything is OK
    CONN_PK_ERR, // Connection Private/Public Key Error
    CONN_VER_ERR, // Connection Version Error
    CONN_HASH_ERR, // Connection HASH error
    CONN_REFUSED, // Connection refused
    CONN_TIMEOUT, // Connection Timeout
    PK_PERM_DENIED, // File Private/Public Key Permision Denied to read the file and load it to the connector
    CONF_PERM_DENIED // Configuration File Permission Denied
};

int tf_send(tf_package package);

int tf_receive(tf_package package);

int tf_connect(char *, uint16_t);

int is_bigendian();

void build_package();

#endif