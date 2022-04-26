/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef XS_IMEOPC1_H
#define XS_IMEOPC1_H

#include <xs_ime/xs_imeopc1.h> 

/* Operation code for set organizing unit. */
#define OPC_UNIT 0x1
/* Operation code for set user. */
#define OPC_USR 0x2
/* Operation code for set up various aspect. */
#define OPC_SETUP 0x4
/* Operation code for send message. */
#define OPC_SNDMSG 0x5
/* Operation code for set time interval. */
#define OPC_TIMER 0x7

/* Set interval to search for new messages. */
void opc_timer(void);
/* Set organizing unit. */
void opc_orgunit(void);
/* Set user in the organizing unit. */
void opc_usr(void);
/* Send message to user(s). This is done in the server organizing unit. */
void opc_sndmsg(void);
/* Set up. */
void opc_setup(void);
/* XS_IMEOPC1 clean up. */
void opc_cleanup(void);
/* Send message to user. This is done by sending the data to the client. */
void opc_sndmsg2(void);

#endif
