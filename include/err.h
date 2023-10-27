/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef ERR_H
#define ERR_H

/* Strings for error codes to send with command failed status. */

/* Access denied. What device or entity depends on the context. */ 
#define CMD_EACCESS "1 : Access denied to location."
/* Invalid protocol for initialize communication. */
#define EPROTO_BADVER "2 : Incompatible protocol."
/* Invalid hash for original application on the client-side. */
#define EPROTO_BADHASH "3 : Invalid hash string."
/* Directory specified for creation already exist. */
#define CMD_EDIREXIST "4 : Directory already exist."
/* Failed adding new notification listening. */
#define CMD_EADDNTFY "5 : Error adding new notification to listen."
/* Failed due notification path list empty. */
#define CMD_ESTARTNTFY "6 : Notify path list empty."
/* Internal error in the file transfer programming interface. */
#define CMD_EFAPINTE "7 : File transfer API error."
/* The requested file to transmit is a directory. */
#define CMD_EISDIR "8 : Requested file is a directory."
/* The requested file does not exist. */
#define CMD_EFILENOENT "9 : File does not exist."
/* Directory does not exist. */
#define CMD_ENOTDIR "10 : Directory does not exist."
/* Failed making temp file. */
#define CMD_EMKTMP "11 : Failed to make a temporary file."
/* File already exist */
#define CMD_EFILEXIST "12 : File already exist."
/* Defined quota for TF daemon exceeded. */
#define CMD_EQUOTA "13 : Quota exceeded."
/* Internal memory error while doing requested task. */
#define CMD_EMEMINT "14 : Internal memory error."
/* Directory to remove can't be deleted. */
#define CMD_EDIRSTILL "15 : Directory to remove still exist."
/* Missing path from syntax command. */
#define CMD_EPARAM "16 : Missing parameter from command." 
/* Path to source file does not exist. */
#define CMD_ESRCFILE "17 : Source file does not exist."
/* General error while linking the new file. */
#define CMD_ECPYERR "18 : Copy general error."
/* Directory linking are not allowd. */
#define CMD_ELINKDIR "19 : Directory can't be linked."
/* specified source path is not a directory. */
#define CMD_ESRCNODIR "20 : Source path is not a directory."
/* Failed to create directory in the tree. */
#define CMD_EDIRTREE "21 : Error replicating directory tree."
/* Error with the memory pool request for more core. */
#define CMD_ELSRERR "22 : Memory error while creating space for path."
/* Specified source path isn't a file. */
#define CMD_ESRCNOFILE "23 : Source path is not a file." 
/* Error ocurred creating new file. */
#define CMD_ETOUCH "24 : Error creating new file."
/* Error with the rsa encryption key. */
#define EPROTO_BADKEY "25 : Bad public rsa encryption key."
/* The date specified is not representable. */
#define CMD_EBADDATE "26 : Date is not representable."
/* Update file or directory timestamps failed. */
#define CMD_EBADUPD "27 : Timestamp updating failed."
/* Invalid characters in lock token for filename. */
#define CMD_EINVLCK "28 : Invalid characters in lock token."
/* The specified source path does not exist. */
#define CMD_ESRCPATH "29 : Source path does not exist."
/* The directory is already locked. */
#define CMD_ELOCKED "30 : Directory is locked."
/* Invalid renaming operation. */
#define CMD_ERENAM "31 : Invalid renaming operation."
/* Failed setting TCP keep alive parameters. */
#define CMD_EKEEPALIVE "32 : Failed to set TCP keepalive parameters."
/* Process unable to create unique key. */
#define CMD_EPROCKEY "33 : Process without unique key."
/* Hi-Performance file interface failed to open file descriptor. */
#define CMD_EHPFFD "34 : H-P interface failed to open file descriptor."
/* Hi-Performance file interface failed to set file position. */
#define CMD_EHPFSEEK "35 : H-P interface failed to set file position."
/* Hi-Performance file interface failed to allocate the buffer. */
#define CMD_EHPFBUF "36 : H-P interface failed to allocate the buffer."
/* Unable to set real user-id. */
#define CMD_ELOGIN "37 : Login failed."
/* Invalid command execution before process sets real user-id. */
#define EPROTO_ENOTLOGGED "38 : Invalid command before login."
/* Trying to login a process that already has logged in. */
#define CMD_EISLOGGED "39 : The process is already logged."
/* Failed to open the users database file. */
#define CMD_ELOGINDB "40 : Failed to open users database."
/* Failed to change file mode. */
#define CMD_ECHMOD "41 : Failed to change file mode."
/* System user does not exist. */
#define CMD_ECHOWNUSR "42 : System user does not exist."
/* System group does not exist. */
#define CMD_ECHOWNGRP "43 : System group does not exist."
/* Failed to change file ownership. */
#define CMD_ECHOWN "44 : Failed to change file ownership."
/* Failed to make the sha256 hash. */
#define CMD_ESHA256 "45 : Failed to make SHA256 hash."
/* Failed the session key swaping process. */
#define CMD_ENIMGA "46 : Failed to swap the session key."
/* Failed removing secure directory. */
#define CMD_ERMSEC "47 : Failed removing secure directory."
/* Failed to open access control list file in jail directory. */
#define CMD_EJAILACL "48 : Failed to open ACL file in jail directory."
/* Invalid security token in jail directory. */
#define CMD_EJAILTOK "49 : Invalid security token in jail directory."
/* Invalid command execution before daemon injailing in a directory. */
#define EPROTO_ENOTINJAILED "50 : Invalid command before in-jail."
/* Trying to injail a process that already has been injailed. */
#define CMD_EISINJAILED "51 : The process is already in-jailed."
/* Failed trying to retrieve Transfer Load Balancer tuple. */
#define CMD_ETLP "52 : Failed to retrieve TLB tuple."
/* Failed executing LSV2 command. */
#define CMD_ELSRV2 "53 : Failed running LSRV2."
/* Failed to acquire or release a lock from NETLOCK command set. */
#define CMD_ENETLOCK "54 : Failed to acquire or release NETLOCK."
/* Failed to acquire or release the network persistent mutex. */
#define CMD_ENETMUTEX "55 : Failed to acquire or release NETMUT."
/* Failed setting an alias. */
#define CMD_ESETALIAS "56 : Failed setting an alias."
/* Failed setting secure FileSystem security token. */
#define CMD_ESETFSPERM "57: Failed setting secure FS permission."
/* Failed removing secure FileSystem security token. */
#define CMD_EREMFSPERM "58: Failed removing secure FS permission."
/* Failed getting secure FileSystem current permission identity. */
#define CMD_EGETFSPERM "59: Failed getting secure FS permission identity."
/* The directory does not belongs to the secure filesystem. */
#define CMD_EISSECFS "60: The directory does not belongs to secure FS."
/* Unable to use the Folder Locking System. */
#define EPROTO_ELOCKSYS "61: Unable to use Folder Locking System."
/* TestAndSet failed to create the file. This is equivalent to False in the
    classic TestAndSet processor instruction. */
#define CMD_ETASFS "62: Unable to create the requested file."
/* Failed to create a directory recursively. */
#define CMD_ERMKDIR "63: Failed to create directory recursively."
/* Failed to run RUNBASH command. */
#define CMD_ERUNBASH "64: Failed running RUNBASH command."
/* Unable to use the FlyContext System. */
#define CMD_EFLYCONTEXT "65: Unable to use the FlyContext System."

#endif
