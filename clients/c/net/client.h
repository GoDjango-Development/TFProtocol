#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
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

int tf_connect(char *, uint16_t);

int tf_send();

int tf_receive();

tf_package* build_package(); 