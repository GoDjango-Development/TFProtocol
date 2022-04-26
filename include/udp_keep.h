/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef UDP_KEEP
#define UDP_KEEP

#include <netinet/in.h>

/* Code for checking host reachability. */
#define UDP_HOSTCHECK 0
/* Code for checking execution state of communication host process. */
#define UDP_PROCHECK 1
/* Code for checking an out-of-band byte sent by the client side. */
#define UDP_SOCKCHECK 2
/* Ok code to send back to the client. */
#define UDP_OK 1
/* Failed code to send back to the client. */
#define UDP_FAILED 0
/* Code for request ip/port of the Transfer Balancer system. */
#define UDP_TLB 3

/* Parameter name to determine if udp server debugging takes palce. */
#define UDPARGVNAM "udp_debug"

/* Debug UDP server flag. */
extern int udp_debug;

void udpkeep_start(struct sockaddr_in6 *srvaddr);

#endif
