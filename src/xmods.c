/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <xmods.h>
#include <string.h>
#include <cmd.h>
#include <util.h>
#include <tfproto.h>

/* Starting part of the eXtended subsystem 1 command. */
#define CMD_XS1 "XS1_"
/* Command to start the eXtended subsystem IME -Instant Message Extension. */
#define CMD_XS_IME "XSIME_START"
/* Command to enter the eXtended subsystem NTMEX -Native Module Execution-. */
#define CMD_XS_NTMEX "XS_NTMEX"
/* Command to enter the eXtended subsystem SQLITE. */
#define CMD_XS_SQLITE "XS_SQLITE"
/* Command to enter the eXtended subsystem Arbitrary Module Execution (ACE). */
#define CMD_XS_ACE "XS_ACE"
/* Command to enter the eXtended subsystem MYSQL. */
#define CMD_XS_MYSQL "XS_MYSQL"
/* Command to enter the eXtended subsystem POSTGRESQL. */
#define CMD_XS_POSTGRESQL "XS_POSTGRESQL"
/* Command to enter the eXtended subsystem Gateway. */
#define CMD_XS_GATEWAY "XS_GATEWAY"
/* Command to enter the Remote Procedure Call Proxy subsystem. */
#define CMD_XS_RPCPROXY "XS_RPCPROXY"

#ifdef XS1_API
#include <xs1/xs1.h>
#endif

#ifdef XS_IME
#include <xs_ime/xs_ime.h>
#endif

#ifdef XS_NTMEX
#include <xs_ntmex/xs_ntmex.h>
#endif

#ifdef XS_SQLITE_MODULE
#include "xs_sqlite/xs_sqlite.h"
#endif

#ifdef XS_ACE
#include "xs_ace/xs_ace.h"
#endif

#ifdef XS_MYSQL_MODULE
#include "xs_mysql/xs_mysql.h"
#endif

#ifdef XS_POSTGRESQL_MODULE
#include "xs_postgresql/xs_postgresql.h"
#endif

#ifdef XS_GATEWAY
#include "xs_gateway/xs_gateway.h"
#endif

#ifdef XS_RPCPROXY
#include "xs_rpcproxy/xs_rpcproxy.h"
#endif


void run_xmods(const char *cmd)
{
#ifdef XS1_API
    if (strstr(cmd, CMD_XS1)) {
        xs1_parse(cmd);
        return;
    }
#endif
#ifdef XS_IME
    if (strstr(cmd, CMD_XS_IME)) {
        xsime_start();
        return;
    }
#endif
#ifdef XS_NTMEX
    if (strstr(cmd, CMD_XS_NTMEX)) {
        xs_ntmex();
        return;
    }
#endif
#ifdef XS_SQLITE_MODULE
    if (strstr(cmd, CMD_XS_SQLITE)) {
        cmd_ok();
        if (xs_sqlite(readbuf_ex, writebuf_ex, jaildir) == -1) { 
            endcomm();
            return;
        } else
            return;
    }
#endif
#ifdef XS_ACE
    if (strstr(cmd, CMD_XS_ACE)) {
        xs_ace();
        return;
    }
#endif
#ifdef XS_MYSQL_MODULE
    if (!strcmp(cmd, CMD_XS_MYSQL)) {
        cmd_ok();
        if (xs_mysql(readbuf_ex, writebuf_ex, jaildir) == -1) { 
            endcomm();
            return;
        } else
            return;
    }
#endif
#ifdef XS_POSTGRESQL_MODULE
    if (strstr(cmd, CMD_XS_POSTGRESQL)) {
        cmd_ok();
        if (xs_postgresql(readbuf_ex, writebuf_ex, jaildir) == -1) { 
            endcomm();
            return;
        } else
            return;
    }
#endif
#ifdef XS_GATEWAY
    if (strstr(cmd, CMD_XS_GATEWAY)) {
        xs_gateway();
        return;
    }
#endif
#ifdef XS_RPCPROXY
    if (strstr(cmd, CMD_XS_RPCPROXY)) {
        cmd_ok();
        xs_rpcproxy();
        return;
    }
#endif
    cmd_unknown();
}
