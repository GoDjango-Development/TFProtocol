/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef CMD_H
#define CMD_H

#include <tfproto.h>
#include <err.h>
#include <limits.h>
#include <inttypes.h>

/* Command token separator as char. */
#define CMD_SEPCH ' '
/* Command token separator as string. */
#define CMD_SEPSTR " "
/* Command max length. */
#define CMD_NAMELEN 32
/* Response of ok for a command. */
#define CMD_OK "OK"
/* Response failed for a command. */
#define CMD_FAILED "FAILED"
/* Return to client on unknown command by the server. */
#define CMD_UNKNOWN "UNKNOWN"
/* Command for terminate network connection.
    this is send by the client end. */
#define CMD_END "END"
/* Echo command wich return the string next after "echo. " */
#define CMD_ECHO "ECHO"
/* Create directory. */
#define CMD_MKDIR "MKDIR"
/* Stream flow control to indicate continue. */
#define CMD_CONT "CONT"
/* Stream flow control to indicate break. */
#define CMD_BREAK "BREAK"
/* List directory content. */
#define CMD_LS "LS"
/* Add notification listener command for directory changes. */
#define CMD_ADDNTFY "ADDNTFY"
/* Command to start process of listening of directory changes. */
#define CMD_STARTNTFY "STARTNTFY"
/* Command to delete a file */
#define CMD_DEL "DEL"
/* Command to send file to the client. */
#define CMD_RCVFILE "RCVFILE"
/* Command to receive file from the client. */
#define CMD_SNDFILE "SNDFILE"
/* Command to remove directory. Caution it does recursively */
#define CMD_RMDIR "RMDIR"
/* List directory content recursively */
#define CMD_LSR "LSR"
/* Copy file from source to destination inside server by doing link. */
#define CMD_COPY "COPY"
/* Copy directory recursively. Replicate directory tree of the source location
    in the destination. Then link every file as well. No real copy occurs. */
#define CMD_CPDIR "CPDIR"
/* Copy source file in every directory that match the destination pattern. */
#define CMD_XCOPY "XCOPY"
/* Copy source directory in every directory that math destination pattern. */
#define CMD_XCPDIR "XCPDIR"
/* Delete files that match pattern in the specified path. */
#define CMD_XDEL "XDEL"
/* Delete all directory recursively that match indicated name. */
#define CMD_XRMDIR "XRMDIR"
/* Set lock filename to use in directory operations. */
#define CMD_LOCK "LOCK"
/* Create new file in specified directory. */
#define CMD_TOUCH "TOUCH"
/* Get the datetime of the server in unix format. */
#define CMD_DATE "DATE"
/* Get the datetime of the server in human readble format. */
#define CMD_DATEF "DATEF"
/* Convert unix time date to human readble format. */
#define CMD_DTOF "DTOF"
/* Convert human readble format into unix time. */
#define CMD_FTOD "FTOD"
/* Get file or directory statatistics. */
#define CMD_FSTAT "FSTAT"
/* Update file or directory timestamps. */
#define CMD_FUPD "FUPD"
/* Starting part of an eXtended subsystem command. */
#define CMD_XS "XS"
/* Change the meaning of a path atomically. */
#define CMD_RENAM "RENAM"
/* Set tcp keepalive parameters. */
#define CMD_KEEPALIVE "KEEPALIVE"
/* Get unique process key. */
#define CMD_PROCKEY "PROCKEY"
/* Hi-Performance file upload command. */
#define CMD_PUT "PUT"
/* Hi-Performance file download command. */
#define CMD_GET "GET"
/* Get free space in the protocol's directory partition. */
#define CMD_FREESP "FREESP"
/* Hi-Performance file upload command with cancellation points. This command it
    is mostly intended for architectures where multi-thread is not available. */
#define CMD_PUTCAN "PUTCAN"
/* Hi-Performance file download command with cancellation points. This command 
    it is mostly intended for architectures where multi-thread is not
    available. */
#define CMD_GETCAN "GETCAN"
/* Login to set the efective-uid of the process. If the configuration file of
    the daemon has a default user, login is not required. */
#define CMD_LOGIN "LOGIN"
/* Change file permission. */
#define CMD_CHMOD "CHMOD"
/* Change file ownership. */
#define CMD_CHOWN "CHOWN"
/* Get the datetime of the server in unix format. The time resolution is broken
    down in seconds and microseconds. */
#define CMD_UDATE "UDATE"
/* Get the datetime of the server in unix format. The time resolution is broken
    down in seconds and nanoseconds. */
#define CMD_NDATE "NDATE"
/* Make a sha256 checksum to a file. */
#define CMD_SHA256 "SHA256"
/* Generate an arbitrary length sesion key and swap it with the current key. */
#define CMD_NIGMA "NIGMA"
/* Remove secure directory. */
#define CMD_RMSECDIR "RMSECDIR"
/* Injail daemon in a directory. */
#define CMD_INJAIL "INJAIL"
/* Get ip/port tuple from Transfer Load Balancer File. */
#define CMD_TLB "TLB"
/* Simple file download. */
#define CMD_SDOWN "SDOWN"
/* Simple file upload. */
#define CMD_SUP "SUP"
/* Get file size. */
#define CMD_FSIZE "FSIZE"
/* Get file sizes from a list. */
#define CMD_FSIZELS "FSIZELS"
/* List directory content version 2. */
#define CMD_LSV2 "LSV2"
/* List directory content recursively version 2. */
#define CMD_LSRV2 "LSRV2"
/* Get filename type. */
#define CMD_FTYPE "FTYPE"
/* Get filename types from a list. */
#define CMD_FTYPELS "FTYPELS"
/* Get file statistics from a list. */
#define CMD_FSTATLS "FSTATLS"
/* Read file with integrity hash. */
#define CMD_INTREAD "INTREAD"
/* Write file if integrity hash matches. */
#define CMD_INTWRITE "INTWRITE"
/* Lock -blocking- a file with timeout that ends communication. */
#define CMD_NETLOCK "NETLOCK"
/* Unkock a file. */
#define CMD_NETUNLOCK "NETUNLOCK"
/* Try lock -non-blocking- a file with timeout that ends communication. */
#define CMD_NETLOCKTRY "NETLOCK_TRY"
/* Try to acquire a network persistent mutex in non-blocking mode. */
#define CMD_NETMUTACQ_TRY "NETMUTACQ_TRY"
/* Release a network persistent mutex in mode. */
#define CMD_NETMUTREL "NETMUTREL"
/* Set secure filesystem identity. */
#define CMD_SETFSID "SETFSID" 
/* Set secure FileSystem permission. */
#define CMD_SETFSPERM "SETFSPERM"
/* Remove secure FileSystem permission. */
#define CMD_REMFSPERM "REMFSPERM"
/* Get secure FileSystem permission for current identity. */
#define CMD_GETFSPERM "GETFSPERM"
/* Check if directory belongs to tfprotocol secure filesystem. */
#define CMD_ISSECFS "ISSECFS"
/* Folder lock system enforcement. */
#define CMD_LOCKSYS "LOCKSYS"
/* Test and Set instruction, implemented through the filesystem. */
#define CMD_TASFS "TASFS"
/* Makes a new directory recursively. */
#define CMD_RMKDIR "RMKDIR"
/* Get tfprotocol instance current time-zone. */
#define CMD_GETTZ "GETTZ"
/* Set tfprotocol instance current time-zone. */
#define CMD_SETTZ "SETTZ"
/* Get time of tfprotocol instance at current time-zone. */
#define CMD_LOCALTIME "LOCALTIME"
/* Get time of a particular time-zone. */
#define CMD_DATEFTZ "DATEFTZ"
/* List directory content version 2 and download. */
#define CMD_LSV2DOWN "LSV2DOWN"
/* List directory content recursively version 2 and download. */
#define CMD_LSRV2DOWN "LSRV2DOWN"
/* Fast add and run a new functionality at server side. */
#define CMD_RUNBASH "RUNBASH"
/* Folder jailing and recursively creation system enforcement. */
#define CMD_FLYCONTEXT "FLYCONTEXT"
/* Turn ON AES Encryption. */
#define CMD_GOAES "GOAES"
/* Turn ON TFProtocol default encryption. */
#define CMD_TFPCRYPTO "TFPCRYPTO"
/* Generate UUID */
#define CMD_GENUUID "GENUUID"
/* Get a Fast Access Interface Token. */
#define CMD_FAITOK "FAITOK"
/* Get a FAI token maximun quanta. */
#define CMD_FAIMQ "FAIMQ"
/* Turn on Client Impersonation Avoidance */
#define CMD_CIAON "CIAON"
/* Turn off Client Impersonation Avoidance */
#define CMD_CIAOFF "CIAOFF"

/* Lock filename. */
extern char lcknam[PATH_MAX];
/* Logged flag. */
extern int logged;

/* Get cmd sent by client. Call endcomm in case of socket failed and return 
    -1. */
int getcmd(void);
/* Parse command string and execute according function. */
void cmd_parse(void);
/* Write down to the socket "ok" result from previous command. */
void cmd_ok(void);
/* Write down to the socket "failed" result from previous command. */
void cmd_fail(const char *arg);
/* Process echo command. */
void cmd_echo(void);
/* Return unknown command to client. */
void cmd_unknown(void);
/* Make directory. */
void cmd_mkdir(void);
/* List directory. */
void cmd_ls(void);
/* Command to send file from server to client. */
void cmd_sendfile(void);
/* Command to receive file from client. */
void cmd_rcvfile(void);
/* Command to delete a file. */
void cmd_del(void);
/* Remove directory recursively. */
void cmd_rmdir(void);
/* List directory recursively. */
void cmd_lsr(void);
/* Copy file from source to destination inside the server. This actually link
    the filename to a new location, instead of copy the file data chunks. */
void cmd_copy(void);
/* Copy directory recursively. Replicate every directory in the destination
    location and link every file as well to corresponding location. */
void cmd_cpdir(void);
/* Copy soruce file to every destination in the directory tree of daemon 
    working directory that match the pattern set in destination path. The name
    of the newly created file must be specified in the first parameter of the
    command. */
void cmd_xcopy(void);
/* Copy source directory recursively into every directory in the tree that
    match destination path. The name of the newly created directory must be
    specified in the first parameter of the command. */
void cmd_xcpdir(void);
/* Delete all files that match specified name in path recursively. */
void cmd_xdel(void);
/* Delete all directory recursively in the specified path that match the name
    parameter parameter. */
void cmd_xrmdir(void);
/* Set lock filename to use in directory operations. */
void cmd_lock(void);
/* Create new file in the specified directory. */
void cmd_touch(void);
/* Get the server datetime in unix format. Number of seconds elapsed since
    the epoch. 00:00:00 Jan 1 1970. */
void cmd_date(void);
/* Get the server datetime in human readble format yyyy-mm-dd HH:MM:SS UTC .*/
void cmd_datef(void);
/* Convert date in unix format -timestamp- to human readble format. */
void cmd_dtof(void);
/* Convert date in human readble format yyyy-mm-dd HH:MM:SS UTC into unix
    timestamp format. */
void cmd_ftod(void);
/* Get statistics of a file or directory. "D | F | U" "FILE SIEZE" 
    "LAST ACCESS" "LAST MODIFICATION" */
void cmd_fstat(void);
/* Update file or directory timestamps to current time. */
void cmd_fupd(void);
/* Change path's meaning atomically. */
void cmd_renam(void);
/* Set TCP keepalive parameters. */
void cmd_keepalive(void);
/* Get unique process key. */
void cmd_prockey(void);
/* Hi-Performance file upload command. */
void cmd_put(void);
/* Hi-Performance file downlaod command. */
void cmd_get(void);
/* Get free space in the protocol's directory partition. */
void cmd_freesp(void);
/* Hi-Performance file upload command with cancellation points. */
void cmd_putcan(void);
/* Hi-Performance file download command with cancellation points. */
void cmd_getcan(void);
/* Extract command from comm buffer. */
void excmd(const char *src, char *cmd);
/* Set the real user-id of the process. */
void cmd_login(void);
/* Change file permission. */
void cmd_chmod(void);
/* Change file ownership. */
void cmd_chown(void);
/* Broken-down time of the server in seconds and microseconds. */
void cmd_udate(void);
/* Broken-down time of the server in seconds and nanoseconds. */
void cmd_ndate(void);
/* Make sha256 checksum to a file. */
void cmd_sha256(void);
/* Generate an arbitrary length sesion key and swap it with the current key. */
void cmd_nigma(void);
/* Remove secure directory. */
void cmd_rmsecdir(void);
/* Injail tfproto daemon in a user-land directory. */
void cmd_injail(void);
/* GET ip/port tuple from tlb file. */
void cmd_tlb(void);
/* Simple file download command. */
void cmd_sdown(void);
/* Simple file upload command. */
void cmd_sup(void);
/* Get file size. */
void cmd_fsize(void);
/* Get file sizes from a list. */
void cmd_fsizels(void);
/* List directory version 2. */
void cmd_lsrv2(int mode);
/* Filename type. */
void cmd_ftype(void);
/* Get filename types from a list. */
void cmd_ftypels(void);
/* Get file stats from a list. */
void cmd_fstatls(void);
/* Acquire resource locking by exclusivelly creating a file. */
void cmd_exclock(void);
/* Release resource locking by deleting the created file. */
void cmd_exclunlock(void);
/* Read file with integrity hash. */
void cmd_intread(void);
/* Write file if integrity hash matches. */
void cmd_intwrite(void);
/* Lock -blocking- a file with timeout that ends communication. */
void cmd_netlock(void);
/* Unkock a file. */
void cmd_netunlock(void);
/* Try lock -non-blocking- a file with timeout that ends communication. */
void cmd_netlocktry(void);
/* Try to acquire a network persistent mutex in non-blocking mode. */
void cmd_netmutacqtry(void);
/* Release a network persistent mutex in mode. */
void cmd_netmutrel(void);
/* Set secure filesystem identity. */
void cmd_setfsid(void);
/* Set secure FileSystem permission. */
void cmd_setfsperm(void);
/* Remove secure FileSystem permission. */
void cmd_remfsperm(void);
/* Get secure FileSystem permission for current identity. */
void cmd_getfsperm(void);
/* Check if directory belongs to tfprotocol secure filesystem. */
void cmd_issecfs(void);
/* Folder lock system enforcement. */
void cmd_locksys(void);
/* Test and Set instruction, atomically implemented through the filesystem. */
void cmd_tasfs(void);
/* Makes a new directory recursively. */
void cmd_rmkdir(void);
/* Get tfprotocol instance current time-zone. */
void cmd_gettz(void);
/* Set tfprotocol instance current time-zone. */
void cmd_settz(void);
/* Get time of tfprotocol instance at current time-zone. */
void cmd_localtime(void);
/* Get time of a particular time-zone. */
void cmd_dateftz(void);
/* List directory version 2 and download the list. */
void cmd_lsrv2down(int mode);
/* Fast add and run a new functionality at server side. */
void cmd_runbash(void);
/* Folder jailing and recursively creation system enforcement. */
void cmd_flycontext(void);
/* Turn ON AES Encryption. */
void cmd_goaes(void);
/* Turn ON TFProtocol default encryption. */
void cmd_tfpcrypto(void);
/* Generate UUID. */
void cmd_genuuid(void);
/* Get a Fast Access Interface Token. */
void cmd_faitok(void);
/* Get a FAI token maximun quanta. */
void cmd_faimq(void);
/* Turn on Client Impersonation Avoidance */
void cmd_ciaon(void);
/* Turn off Client Impersonation Avoidance */
void cmd_ciaoff(void);

#endif
