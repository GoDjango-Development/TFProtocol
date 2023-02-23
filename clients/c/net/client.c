struct{
    int socket;
    char* session_key;
    int header_size;
} tfprotocol;

typedef struct {
    int header;
    char* body;
} tf_package;

int is_bigendian(){
    int value = 1; 
    char *pt = (char *) &value;
    if (*pt == 1)
        return 0;
    else
        return 1;
}

/* Convert a tfprotocol into a TF Package*/
void build_package(char* data, tf_package* package){
    package->header = strlen(data);
    package->body = data;
}


int tf_send(tf_package package){
    int header = package.header; // For saving the variable as its possible that later the swapbo modify this value...
    if(!is_bigendian()){
        swapbo32(package.header);
    }
    int res = send(tfprotocol.socket, &package.header, sizeof(package.header), 0);
    res = send(tfprotocol.socket, package.body, header, 0);
    return 0;
}

int tf_receive(tf_package package){
    int res = recv(tfprotocol.socket, package.body, tfprotocol.header_size, 0); // Right now package.body is the header
    int header = *((int*)package.body);
    if(!is_bigendian()){
        swapbo32(header);
    }
    printf("%d\n", header);
    res = recv(tfprotocol.socket, package.body, header, 0);
    return 0;
}

int tf_connect(char *addr, uint16_t port){
    tfprotocol.socket = socket(PF_INET, SOCK_STREAM, 0); /* We may also use here AF_IPV4 but as libc describes letting that in 0 allows the system to 
    automatically choose one */
    int res;
    if (tfprotocol.socket < 0){
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
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr = *(struct in_addr *) server_addr->h_addr;
    res = connect(tfprotocol.socket, (struct sockaddr*) &sock_addr, sizeof(sock_addr));
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

    tf_package pkg_send;
    build_package("0.0", &pkg_send);
    tf_send(pkg_send);
    tf_package pkg_recv;
    tf_receive(pkg_recv);
    //printf("%d\n", pkg_recv.header);
    //printf("%s\n", pkg_recv.body);
    return 0;
}