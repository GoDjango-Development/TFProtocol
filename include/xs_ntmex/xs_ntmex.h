
/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef XS_NTMEX_H
#define XS_NTMEX_H

#define XS_NTMEX_EKEYREAD "1 : Failed to read key's database."
#define XS_NTMEX_EKEYNOTSET "2 : Failed to set the key."
#define XS_NTMEX_EKEYSET "3 : Key already set."
#define XS_NTMEX_ERUN "4 : Entry-point not assigned."
#define XS_NTMEX_EPERM "5 : Access key have not installed yet."
#define XS_NTMEX_ELOAD "6 : Failed to load the module."
#define XS_NTMEX_ENFO "7 : Information unavailable."
#define XS_NTMEX_EACL "8 : Access Control List denied."

/* Install access key. */
#define XS_NTMEX_INSKEY "INSKEY"
/* Exit module without free resources. */
#define XS_NTMEX_GOBACK "GOBACK"
/* Exit module and free resources. */
#define XS_NTMEX_EXIT "EXIT"
/* Load a shared-object. */
#define XS_NTMEX_LOAD "LOAD"
/* Run the entry-point of the loaded shared-object. */
#define XS_NTMEX_RUN "RUN"
/* Retreive system, machine, and so on. information. */
#define XS_NTMEX_SYSNFO "SYSNFO"

/* Entry point to the NTMEX module. */
void xs_ntmex(void);
/* Install access key. */
void xs_ntmex_inskey(void);
/* Exit module without free resources. */
void xs_ntmex_goback(void);
/* Exit module and free resources. */
void xs_ntmex_exit(void);
/* Load a shared-object. */
void xs_ntmex_load(void);
/* Run the entry-point of the loaded shared-object. */
void xs_ntmex_run(void);
/* Retreive system information. */
void xs_ntmex_sysnfo(void);

#endif
