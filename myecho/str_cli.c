#include "myecho.h"

void str_cli(FILE *fp, int sockfd) {
	char sendline[LINE_MAX], recvline[LINE_MAX];
	ssize_t ret;

	while (fgets(sendline, LINE_MAX, fp) != NULL) {
		if (writen(sockfd, sendline, strlen(sendline)) == -1)
			err_quit("str_cli write error");

		if ( (ret = readline(sockfd, recvline, LINE_MAX)) == -1)
			err_quit("str_cli readline error");
		else if (ret == 0)
			err_quit("str_cli: server terminated prematurely");
		fputs(recvline, stdout);
	}
}
