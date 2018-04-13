#include "../myecho.h"

ssize_t writen(int fd, const void *buf, size_t n) {
	size_t nleft = n;
	ssize_t nwritten;
	const char* pbuf = (const char *)buf;

	while (0 < nleft) {
		if ( (nwritten = write(fd, pbuf, nleft)) == -1) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else {
			nleft -= nwritten;
			pbuf += nwritten;
		}
	}
	return n;
}
