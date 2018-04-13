#ifndef MYECHO_H_
#define MYECHO_H_

#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include "err_handlers/err_handlers.h"

#define SERV_PORT	9000
#define LISTENQ 	1024
#define TEST_SIGCHLD	0

typedef void Sigfunc(int);

extern ssize_t writen(int fd, const void *buf, size_t n);
extern ssize_t readline(int fd, void *buf, size_t maxline);
extern void str_cli(FILE *fp, int sockfd);
extern void str_echo(int sockfd);
extern Sigfunc *mysignal(int signo, Sigfunc *func);
extern void sig_chld(int signo);

#endif /* MYECHO_H_ */
