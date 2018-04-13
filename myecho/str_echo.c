#include "myecho.h"

void str_echo(int sockfd) {
	ssize_t n;
	char buf[LINE_MAX];

	while ( (n = read(sockfd, buf, LINE_MAX)) != 0) {
		if (n == -1) {
			/* 返回-1并置errno=EINTR，表示在读取任何字节之前接收到信号。read可以重新执行 */
			if (errno == EINTR)
				continue;
			else
				err_sys("str_echo read error");
		} else {
			if (writen(sockfd, buf, n) == -1)
				err_quit("str_echo write error");
		}
	}
}
