# UNIX网络编程socket    

## 1. 基本概念    

### 1.1 头文件    
  | 数据结构/函数						| 头文件	|
  | :---:								| :---:		|
  | struct sockaddr_in					| netinet/in.h	|
  | struct sockaddr						| sys/socket.h	|
  | socklen_t							| unistd.h		|
  | htons()/htonl()/ntohs()/ntohl()		|	netinet/in.h	|
  | inet_pton()/inet_ntop()				| arpa/inet.h	|


### 1.2 多进程并发服务器    
  服务器父进程调用`accept`从已完成连接队列头返回下一个已完成连接的已连接套接字(connected socket)描述符`connfd`，然后调用`fork`创建一个子进程来服务该连接客户。    
  1. 为什么子进程能够服务已连接客户    
    父进程fork之前打开的所有文件描述符在fork返回之后由子进程共享，所以父进程调用`socket`创建的监听套接字(listening socket)描述符`listenfd`，和该已连接套接字描述符`connfd`就在父进程与子进程之间共享。通常情况下，**子进程读写这个已连接套接字connfd，父进程关闭这个已连接套接字；父进程再次调用accept使用监听套接字listenfd，子进程则关闭该监听套接字**。    
  2. 为什么父进程对connfd调用close没有终止它与客户端的连接    
    对于一个TCP套接字调用close会导致发送一个FIN分节，accpet返回之后父进程fork一个子进程服务已连接的客户，父进程close已连接套接字connfd，为什么没有导致子进程与客户端连接的终止？同样的，子进程close监听套接字listenfd，为什么没有导致父进程从LISTEN状态转变为CLOSED状态？    
	这是因为每个文件或套接字都有一个引用计数，在文件表中维护，它是当前打开着的引用该文件或套接字的描述符的个数。调用close只是将该引用计数减1，该文件或套接字真正的清理和释放要等到其引用计数值到达0时才发生。    


## 2. 回射服务器    
  1. 多进程并发服务器，不处理服务器子进程退出    
    客户fgets阻塞读stdin，然后writen写入到sockfd；服务器fork子进程read阻塞读connfd，然后writen回射给客户；客户readline阻塞读sockfd，然后fputs写入stdout。    
	服务器子进程退出时，会给父进程发送一个`SIGCHLD`信号。由于服务器父进程要一直调用accept，接受客户连接，所以不能调用wait等待子进程退出。所以如果服务器父进程不处理`SIGCHLD`信号，子进程就进入僵尸状态。    

  2. 多进程并发服务器，处理SIGCHLD信号    
      1. 信号管理    
	      1. 基本信号管理    
	        C标准定义的`signal`函数，早于POSIX标准，不同的实现提供不同的信号语义以达成向后兼容，不符合POSIX语义。    
			```c
			#include <signal.h>

			typedef void (*sighandler_t)(int);
			sighandler_t signal(int signo, sighandler_t *handler);
			```

	      2. 高级信号管理    
	        POSIX定义了`sigaction`系统调用。    
			```c
			#include <signal.h>

			int sigaction(int signo, const struct sigaction *act, struct sigaction *oldact)
			```

			调用sigaction会改变由signo表示的信号的行为，signo是除`SIGKILL`和`SIGSTOP`外的任何值。`act`非空，将该信号的当前行为替换成参数act指定的行为。`oldact`非空，在其中存储先前(或者是当前的，如果act非空)指定的信号行为。    
			结构体`struct sigaction`支持细粒度控制信号：    
			```c
			struct sigaction {
				void (*sa_handler)(int);	/* signal handler or action */
				void (*sa_sigaction)(int, siginfo_t *, void *);		/* 新的表示如何执行信号处理函数 */
				sigset_t sa_mask;	/* 执行信号时被阻塞的信号集 */
				int sa_flags;	/* flags */
				void (*sa_restore)(void);	/* obsolete and non-POSIX */
			}
			```

			如果`sa_flags`设置`SA_SIGINFO`标志，则由`sa_sigaction`来决定如何执行信号处理，提供有关该信号的更多信息和功能；否则，使用`sa_handler`处理信号，与C标准`signal`函数原型相同。    

			信号集类型`sigset_t`表示一组信号集合，定义下列函数管理信号集：    
			```c
			#include <signal.h>

			int sigemptyset(sigset_t *set);
			int sigfillset(sigset *set);
			int sigaddset(sigset *set, int signo);
			int sigdelset(sigset *set, int signo);
			int sigismember(const sigset *set, int signo);
			```

      2. 父进程阻塞于accept慢系统调用时处理SIGCHLD信号可能导致父进程中止    
	    当SIGCHLD信号递交时，父进程阻塞于accept调用，accept是慢系统调用，内核会使accept返回一个EINTR错误(被中断的系统调用)。如果父进程不处理这个错误，就会中止。而这里父进程没有中止，是因为在注册信号处理函数mysignal中设置了`SA_RESTART`标志，内核自动重启被中断的accept调用。不过为了便于移植，必须为accept处理EINTR错误。    
	    慢系统调用的基本原则：当阻塞于某个慢系统调用的一个进程捕获某个信号且相应信号处理函数返回时，该系统调用可能返回一个EINTR错误，我们必须处理慢系统调用返回的EINTR错误。    
	
	  3. Unix信号是不排队的，所以使用非阻塞waitpid处理SIGCHLD信号    
        Unix信号默认是不排队的。也就是说，如果一个信号在被阻塞期间产生了一次或多次，那么该信号被解阻塞通常只递交一次。如果多个SIGCHLD信号在信号处理函数sig_chld执行之前产生，由于信号是不排队的，所以sig_chld函数只执行一次，仍会出现僵尸进程。信号处理函数应改为使用waitpid，以获取所有已终止子进程的状态，同时指定`WNOHANG`选项，告知waitpid在有尚未终止的子进程在运行时不要阻塞，从而直接从信号处理函数返回。    

