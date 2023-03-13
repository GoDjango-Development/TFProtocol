/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <log.h>
#include <stdio.h>

/* Strings for using with syslog */
#define LGC_CRITICAL_STR "[ CRITICAL ]"
#define LGC_INFO_STR "[ INFO ]"
#define LGC_WARNING_STR "[ WARNING ]"

/* non-zero if tfprotocol is running in daemon mode. */
int dmode;

void wrlog(const char *msg, enum logcat cat)
{
    char *catpt = NULL;
    switch (cat) {
    case LGC_CRITICAL:
        catpt = LGC_CRITICAL_STR;
        break;
    case LGC_INFO:
        catpt = LGC_INFO_STR;
        break;
    case LGC_WARNING:
        catpt = LGC_WARNING_STR;
        break;
    }
    syslog(LOG_INFO | LOG_DAEMON, "%s %d %s %s","PID: :", getpid(), msg, catpt);
    if (!dmode)
        printf("%s %s %s %d\n", catpt, msg, "PID: ", getpid());
    closelog();
}

