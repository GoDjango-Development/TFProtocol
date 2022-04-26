/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef XMODS_H
#define XMODS_H

/* Undefine this macro to remove the XS1 subsystem. */
#define XS1_API
/* Undefine this macro to remove the XS_IME subsystem. */
#define XS_IME
/* Undefine this macro to remove the XS_NTMEX subsystem. */
#define XS_NTMEX
/* Undefine this macro to remove the XS_SQLITE subsystem. */
#define XS_SQLITE_MODULE
/* Undefine this macro to remove the XS_ACE subsystem. */
#define XS_ACE
/* Undefine this macro to remove the XS_MYSQL subsystem. */
#define XS_MYSQL_MODULE
/* Undefine this macro to remove the XS_POSTGRESQL subsystem. */
#define XS_POSTGRESQL_MODULE
/* Undefine this macro to remove the XS_GATEWAY subsystem. */
#define XS_GATEWAY

void run_xmods(const char *cmd);

#endif
