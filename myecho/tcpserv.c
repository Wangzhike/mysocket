#include "myecho.h"

int main(int argc, char **argv)
{
	int listenfd, connfd;	/* 监听套接字，已连接套接字 */
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in servaddr, cliaddr;

	/* 创建一个socket套接字 */
	if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err_sys("create socket failed for server");
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	/* 将本地协议地址绑定到套接字 */
	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
		err_sys("bind to port %d failed", SERV_PORT);
	/* 将未连接的套接字转换成一个被动打开(passive open)的套接字 */
	if (listen(listenfd, LISTENQ) == -1)
		err_sys("listen faild");

	if (mysignal(SIGCHLD, sig_chld) == SIG_ERR)		/* must call waitpid() */
		err_quit("register signal handler for SIGCHLD failed");

	while (1) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1) {
			if (errno == EINTR)
				continue;		/* 处理慢系统调用由于处理SIGCHLD信号返回时的EINTR错误 */
			else
				err_sys("accept failed");
		}

		/* 创建一个子进程服务已连接的客户 */
		if ( (childpid = fork()) == 0) {
			close(listenfd);	/* 子进程关闭监听套接字 */
			/* do something */
			str_echo(connfd);
			exit(EXIT_SUCCESS);
		}
		close(connfd);	/* 父进程关闭已连接套接字 */
	}

}
