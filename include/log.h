/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef LOG_H
#define LOG_H

#define ELOGDINIT "TF Protocol daemon failed reading init file."
#define ELOGDFORK "TF Protocol daemon failed forking the process."
#define ELOGDDIR "TF Protocol daemon database directory error."
#define ELOGDARG "TF Protocol daemon insufficient arguments."
#define ELOGDCSOCK "TF Protocol daemon failed to create the socket."
#define ELOGDBSOCK "TF Protocol daemon failed to bind the socket."
#define ELOGDLSOCK "TF Protocol daemon failed to listen in the socket."
#define ELOGDASOCK "TF Protocol daemon failed to accept new connection."
#define SLOGDINIT "TF Protocol daemon initialized successfully."
#define SLOGDRUNNING "TF Protocol daemon running successfully."
#define SLOGDSRVS "TF Protocol daemon start to listen for connections."
#define SLOGSIGINT "TF Protocol daemon exiting due SIGINT receive."
#define ELOGKEEPALIVE "TF Protocol daemon falied to set TCP keepalive timeouts."
#define ELOGOOBTHREAD "TF Protocol daemon failed to initialize OOB thread."
#define ELOGDBSOCKUDP "TF Protocol daemon failed to bind UDP socket."
#define ELOGDCSOCKDGRAM "TF Protocol daemon failed to create DATAGRAM socket."
#define ELOGINITFLAGS "TF Protocol daemon invalid flags."
#define ELOGSTACKSIZE "TF Protocol daemon failed to set stack size."

/* Enum to indicate the category of the log action */
enum logcat { LGC_CRITICAL, LGC_INFO, LGC_WARNING };

/* This function write to syslog */
void wrlog(const char *msg, enum logcat cat);

#endif
