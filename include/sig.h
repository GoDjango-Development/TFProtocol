/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef SIG_H
#define SIG_H

#include <signal.h>

/* Signal mask. */
extern sigset_t mask;

/* Set signal handler for the specified "signo" signal */
void setsighandler(int signo, void (*handler)(int));
/* Signal handler for SIGINT signal. */
void sigint(int signo);
/* Signal handler for SIGUSR1 signal. This reloads daemon configuration. */
void sigusr1(int signo);
/* Reload TFProtocol configuration instance. */
void rlwait(char *const *argv, pid_t pid);

#endif 
