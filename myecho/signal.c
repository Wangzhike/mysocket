#include "myecho.h"

/*
 * 对sigaction进行类似signal封装的符合POSIX语义的信号处理函数
 */
Sigfunc *mysignal(int signo, Sigfunc *func) {
	struct sigaction act, oldact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
	#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
	#endif
	} else {
	#ifdef	SA_RESTART
		/* 该标志可以使被信号中断的系统调用以BSD风格重启 */
		act.sa_flags |= SA_RESTART;		/* SVR4, 4.4BSD*/
	#endif
	}
	if (sigaction(signo, &act, &oldact) == -1)
		return SIG_ERR;
	return oldact.sa_handler;
}

/* 处理SIGCHLD信号 */
void sig_chld(int signo) {
	pid_t pid;
	int stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return ;
}
