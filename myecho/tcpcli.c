#include "myecho.h"

#if TEST_SIGCHLD
#define NUM_CLIENT	5
int main(int argc, char **argv)
{
	int i, sockfd[NUM_CLIENT];
	struct sockaddr_in servaddr;

	if (argc != 2)
		err_quit("usage: %s <IPaddress>");
	for (i = 0; i < NUM_CLIENT; ++i) {
		if ( (sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			err_sys("create socket for client failed");
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(SERV_PORT);
		if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr.s_addr) == -1)
			err_sys("tcpclient inet_pton error");
		/* connect会激发TCP的三次握手，而且仅在连接建立成功或出错时才返回 */
		if (connect(sockfd[i], (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
			err_sys("client connect error for server %s: %d", argv[1], SERV_PORT);
	}
	/* do it all */
	str_cli(stdin, sockfd[0]);
	exit(EXIT_SUCCESS);
}
#else
int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;

	if (argc != 2)
		err_quit("usage: %s <IPaddress>");
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err_sys("create socket for client failed");
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr.s_addr) == -1)
		err_sys("tcpclient inet_pton error");
	/* connect会激发TCP的三次握手，而且仅在连接建立成功或出错时才返回 */
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
		err_sys("client connect error for server %s: %d", argv[1], SERV_PORT);

	/* do it all */
	str_cli(stdin, sockfd);
	exit(EXIT_SUCCESS);
}
#endif 
