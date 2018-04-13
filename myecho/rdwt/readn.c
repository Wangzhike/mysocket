#include "../myecho.h"

/*
 * 原始的readline函数需要像fgets那样工作，当读入到一个换行符并存储到
 * 缓冲区后就不再读取；如果缓冲区内存储的字符数达到buffer_size-1个时
 * 也停止读取。任何一种情况下，都会插入一个NUL字节('\0')到缓冲区所存储
 * 数据的末尾，使其成为一个字符串。
 * 而readline调用read实现，每次读一个字节，不断检测当前字符是否为换行，
 * 如果是就停止读取。
 * 这样做的效果就是原始的readline极其低效！那为什么不直接使用fgets函数
 * 呢？fgets函数和read不同，read是内核缓冲I/O，而fgets是用户缓冲I/O，
 * fgets使用标准I/O库(stdio)的缓冲区，而stdio缓冲区在用户态，并且不由
 * 我们控制，所以其状态是不可见的，而这会引发很多问题。
 * 所以我们采取折中的办法，建立自己的用户缓冲区，这样能够查看缓冲区中
 * 的内容。
 */

static int read_cnt;
static char *read_ptr;
static char read_buf[LINE_MAX];

static ssize_t my_read(int fd, char *ptr) {
	if (read_cnt <= 0) {
		again:
			if ( (read_cnt = read(fd,read_buf, sizeof(read_buf))) == -1) {
				if (errno == EINTR)
					goto again;
				else
					return -1;
			} else if (read_cnt == 0)
				return 0;
			read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return 1;
}

ssize_t readline(int fd, void *buf, size_t maxlen) {
	ssize_t rc;
	size_t n;
	char c, *pbuf = (char*)buf;
	for (n = 1; n < maxlen; ++n) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*pbuf++ = c;
			if (c == '\n')
				break;		/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*pbuf = 0;
			return n-1;		/* EOF, n - 1 bytes were read */
		} else {
			return -1;
		}
	}

	*pbuf = 0;		/* null terminate like fgets() */
	return n;
}

ssize_t readlinebuf(void **vptrptr) {
	if (read_cnt)
		*vptrptr = read_buf;
	return read_cnt;
}
