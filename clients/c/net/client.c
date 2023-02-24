struct{
    unsigned short header_size;
    int socket;
    char* session_key;
} tfprotocol;

typedef struct{
    int status_code;
    long size;
    char* status;
    char* payload;
} tf_package;
/*
typedef struct {
    int header;
    char* body;
} tf_package;*/


int is_bigendian(){
    int value = 1; 
    char *pt = (char *) &value;
    if (*pt == 1)
        return 0;
    else
        return 1;
}

/* Convert a tfprotocol into a TF Package*/
void build_package(char* data, long size, tf_package* package){
    if (size == -1){
        package->size = sizeof(data);
        package->payload = data;
    }else{
        package->size = size;
        // TODO: Dynamic Status Code resolver
        package->payload = data;

        package->status_code = 0;
        package->status = "OK";
    }
}
short is_package_ok(tf_package package){
    if (strcmp(package.payload, "OK")==0){
        printf("Everything is OK");
    }
    return package.status_code == 0;
}

int tf_send(tf_package package){
    int header = package.size; // For saving the variable as its possible that later the swapbo modify this value...
    if(!is_bigendian()){
        if(tfprotocol.header_size==SHORT_HEADER) 
            package.size = package.size<<32; // As the value type is long we need to make it ready for swap like it was a 4 bytes int
        swapbo64(package.size);
    }
    int res = send(tfprotocol.socket, &package.size, tfprotocol.header_size, 0);
    res = send(tfprotocol.socket, package.payload, header, 0);
    return 0;
}

int tf_receive(tf_package package){
    package.payload = (char*)malloc(tfprotocol.header_size); // LOL If the memory is not dynamic for some reason below doesnt work.
    int res = recv(tfprotocol.socket, package.payload, tfprotocol.header_size, 0); // Right now package.body is the header
    int header = *((long*)package.payload);
    if(!is_bigendian()){
        if(tfprotocol.header_size==SHORT_HEADER) 
            package.size = package.size<<32; // As the value type is long we need to make it ready for swap like it was a 4 bytes int
        swapbo32(header);
    }
    package.payload = (char*)realloc(package.payload, header);
    res = recv(tfprotocol.socket, package.payload, header, 0);
    printf("%d\n", header);
    printf("%s\n", package.payload);
    tf_package pkg;
    build_package(package.payload, header, &pkg);
    is_package_ok(pkg);
    free(package.payload); // FIXME: This should keep data until its end
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
    build_package("0.0", -1, &pkg_send);
    tf_send(pkg_send);
    tf_package pkg_recv;
    tf_receive(pkg_recv);
    //printf("%d\n", pkg_recv.header);
    //printf("%s\n", pkg_recv.body);
    return 0;
}