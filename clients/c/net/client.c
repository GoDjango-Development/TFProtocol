typedef struct {
    int header;
    char* body;
} tf_package;

int tf_connect(char *addr, uint16_t port){
    int sock = socket(PF_INET, SOCK_STREAM, 0); /* We may also use here AF_IPV4 but as libc describes letting that in 0 allows the system to 
    automatically choose one */
    int res;
    if (sock < 0){
        printf("Something ocurred while trying to create the socket...\n");
        exit(EXIT_FAILURE);
    }
    struct hostent *server_addr = gethostbyname(addr);
    
    if (server_addr == NULL){
        printf("Something ocurred while resolving the hostname...\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = port;
    sock_addr.sin_addr.s_addr = *(in_addr_t *) server_addr->h_addr;
    res = connect(sock, (struct sockaddr*) &sock_addr, sizeof(sock_addr));
    if (res < 0){
        if (res == ETIMEDOUT){
            printf("The connection timeout, please ensure you have a reliable internet connection or you havent a firewall ignoring \
            connections to the given address or the given port\n");
        }else if(res == ECONNREFUSED){
            printf("The server refused to connect this usually means that the servers was reachable but the port is not listen, also can mean \
            a firewall is rejecting the connections, please ensure that the given host and port and correct and there is not firewall.");
        }else if(res == ENETUNREACH){
            printf("Please ensure that your router can route to the given host or ip\n");
        }else if(res == EINPROGRESS || res == EALREADY){
            printf("There is another running operation and the socket is in not blocking mode");
        }
        printf("Cannot connect to the given address \n");
        exit(EXIT_FAILURE);
    }
    return sock;
}

/* Convert a tfprotocol into a TF Package*/
tf_package* build_package(){

}