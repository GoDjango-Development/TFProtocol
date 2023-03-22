/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef NET_H
#define NET_H

/* IPC PIPE to avoid child zombie. */
int zfd[2];

/* Start network listening and client dispatcher into forked process */
void startnet(void);
/* Close listen server socket */
void endnet(void);
/* Avoid possible child zombie process. */
void avoidz(void);

#endif
