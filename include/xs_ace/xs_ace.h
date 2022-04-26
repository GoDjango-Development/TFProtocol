/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef XS_ACE_H
#define XS_ACE_H

/* Buffer type for RUNNL_SZ/RUNBUF_SZ. */
#define XS_BUFTYPE_LN 0
#define XS_BUFTYPE_BUF 1
/* Buffer lenght to the error message. */
#define XS_ERRBFLEN 256
/* Code to indicate successfully command execution or client confirmation. */
#define XS_ACE_OK -127000
/* Code to indicate that there is an error to be readed. */
#define XS_ACE_ERROR -128000
/* Code to indicate that the execution of the module is finished. */
#define XS_ACE_RUNEND -129000
/* Code to tells the server to send SIGKILL to the module been executed. */
#define XS_ACE_SIGKILL -130000
/* Code to tells the server to send SIGTERM to the module been executed. */
#define XS_ACE_SIGTERM -131000
/* Code to tells the server to send SIGUSR to the module been executed. */
#define XS_ACE_SIGUSR1 -132000
/* Code to tells the server to send SIGUSR1 to the module been executed. */
#define XS_ACE_SIGUSR2 -133000
/* Code to tells the server to send SIGHUP to the module been executed. */
#define XS_ACE_SIGHUP -134000
#define XS_ACE_EKEYREAD "1 : Failed to read key's database."
#define XS_ACE_EKEYNOTSET "2 : Failed to set the key."
#define XS_ACE_EKEYSET "3 : Key already set."
#define XS_ACE_ERUN "4 : Failed to run the module."
#define XS_ACE_EPERM "5 : Access key is not installed yet."
#define XS_ACE_EARGS "6 : Failed to set argument list."
#define XS_ACE_EBUF "7 : Failed to create comm buffers."
#define XS_ACE_EWDIR "8 : Failed setting working directory."
#define XS_ACE_EACL "9 : Access Control List denied."

/* Install access key. */
#define XS_ACE_INSKEY "INSKEY"
/* Exit the subsystem. */
#define XS_ACE_EXIT "EXIT"
/* Run the ACE Module. */
#define XS_ACE_RUN "RUN"
/* Run the ACE Module, Variant 'NewLine delimiter'.*/
#define XS_ACE_RUNNL "RUN_NL"
/* Sets the strings arguments array. */
#define XS_ACE_SETARGS "SETARGS"
/* Set the working directory. */
#define XS_ACE_SETWDIR "SETWDIR"
/* Gets line-oriented buffer size of RUN_NL command. */
#define XS_ACE_RUNNL_SZ "RUNNL_SZ"
/* Gets fully-buffered pipe buffer size used in RUN_BUF command. */
#define XS_ACE_RUNBUF_SZ "RUNBUF_SZ"
/* Run the ACE Module, Variant 'Fully-bufferd'.*/
#define XS_ACE_RUNBUF "RUN_BUF"
/* Set the buffer size for the RUN_BUF command. */
#define XS_ACE_SET_RUNBUF "SET_RUNBUF"
/* Set the buffer size for the RUN_NL command. */
#define XS_ACE_SET_RUNNL "SET_RUNNL"
/* Temporary exit the subsystem. */
#define XS_ACE_GOBACK "GOBACK"
/* Run the ACE Module in the background detached from ACE. */
#define XS_ACE_RUNBK "RUN_BK"

extern char xs_errbuf[XS_ERRBFLEN];

/* Enters the ACE subsystem. */
void xs_ace(void);
/* Install access key. */
void xs_ace_inskey(void);
/* Exit the ACE subsystem. */
void xs_ace_exit(void);
/* Run the ACE subsystem. */
void xs_ace_run(void);
/* Run the ACE subsystem, The NewLine delimiter. */
void xs_ace_runnl(void);
/* Sets the strings arguments array. */
void xs_ace_setargs(void);
/* Sets the working directory. */
void xs_ace_setwdir(void);
/* Gets buffer size of RUN_NL or RUN_BUF command. */
void xs_ace_run_bufsz(int buftp);
/* Run the ACE subsystem, The Fully-buffered variant. */
void xs_ace_runbuf(void);
/* Set the buffer size for the RUN_BUF command. */
void xs_ace_set_runbuf(void);
/* Set the buffer size for the RUN_NL command. */
void xs_ace_set_runnl(void);
/* Temporary exit the ACE subsystem. */
void xs_ace_goback(void);
/* Run the ACE subsystem. Background mode. */
void xs_ace_runbk(void);

#endif
