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

### 2.1 多进程并发服务器，不处理服务器子进程退出    
  客户fgets阻塞读stdin，然后writen写入到sockfd；服务器fork子进程read阻塞读connfd，然后writen回射给客户；客户readline阻塞读sockfd，然后fputs写入stdout。    
  服务器子进程退出时，会给父进程发送一个`SIGCHLD`信号。由于服务器父进程要一直调用accept，接受客户连接，所以不能调用wait等待子进程退出。所以如果服务器父进程不处理`SIGCHLD`信号，子进程就进入僵尸状态。    

### 2.2 多进程并发服务器，处理SIGCHLD信号    
  1. 信号管理:    
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

### 2.3 客户进程同时应对sockfd和stdin两个描述符，使用select I/O多路复用，使服务器进程一经终止客户就能检测到    

  启动客户/服务器对，然后杀死服务器子进程，从而模拟服务器进程崩溃的情况(注意这里对应的是服务器进程崩溃，而服务器主机崩溃是另外的情况)。子进程被杀死后，系统发送SIGCHLD信号给服务器父进程，父进程正确处理子进程异常终止，关闭子进程打开的所有文件描述符，从而引发服务器TCP发送一个FIN分节给客户TCP，并响应一个ACK分节。**接下来服务器TCP期待TCP四次挥手的后两个分节，但此时客户进程阻塞在fgets调用上，等待从终端接收一行文本，无法执行readline函数读取服务器TCP发送的FIN分节代表的EOF，所以不能直接退出以向服务器TCP回送FIN分节，所以此时服务器TCP处于CLOSE_WAIT状态，客户TCP处理FIN_WAIT2状态**，可以用netstat命令观察到套接字的状态：    
  ![process of server terminated prematurely](https://github.com/Wangzhike/mysocket/raw/master/myecho/picture/process_of_server_terminated_prematurely.png)    
  本例子的问题在于：当FIN到达套接字时，客户正阻塞在fgets调用上。客户实际上在应对两个描述符——套接字和用户输入，它不能单纯阻塞在这两个源中某个特定源的输入上，而是应该阻塞在其中任何一个源的输入上。这正是select和poll这两个函数的目的之一。    
  对于客户进程需要一种预先告知内核的能力，使得内核一旦发现进程指定的一个或多个I/O条件就绪(输入已准备好被读取，或者描述符已能承载更多的输出)，它就通知进程。这个能力称为I/O复用。I/O复用使进程阻塞在select或poll系统调用上，而不是阻塞在真正的I/O系统调用上。    
  
  I/O复用的典型应用场景：    
      - 客户处理多个描述符(通常是交互式输入和网络套接字)    
	  - TCP服务器既要处理监听套接字listenfd，又要处理已连接套接字connfd    

  select函数：    
      该函数允许进程指示内核等待多个事件(读、写、异常)中的任何一个发生，并只在有一个或多个时间发生或经历一段指定的时间后才唤醒。    
	  ```c
	  #include <sys/select.h>
	  #include <sys/time.h>

	  int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
	  ```

	  `timeout`为NULL，一直阻塞于select调用，直到有一个描述符准备好I/O才返回；`timeout`不为NULL，阻塞一段固定时间，如果没有一个文件描述符准备好，一直等待这么长时间后返回；如果`timeout`值为0，则检查描述符后立即返回。    
	  `readfds`、`writefds`、`exceptfds`为读、写和异常条件的描述符集。select使用描述符集，通常是一个整形数组，其中每个整数中的每一位对应一个描述符。举例来说，假设使用32位整数，那么该数组的第一个元素对应于描述符`0~31`，第二个元素对应于描述符`32~61`，依次类推。实现细节隐藏于`fd_set`数据类型，使用四个宏函数操纵描述符集：    
	  ```c
	  void FD_ZERO(fd_set *set);	/* clear all bits in set */
	  void FD_SET(int fd, fd_set *set);		/* turn on the bit for fd in set */
	  void FD_CLR(int fd, fd_set *set);		/* turn off the bit for fd in set */
	  void FD_ISSET(int fd, fd_set *set);	/* is the bit of fd on in set? */
	  ```

	  对`readfds`、`writefds`、`exceptfds`三个参数中的某一个不感兴趣，则可以把它设为NULL。`nfds`指定待测试的描述符个数，其值为待测试的最大描述符加1，因为描述符是从0开始的。所以，`[0, nfds)`指定描述符的范围，在这个指定范围内，由`readfds`、`writefds`、`exceptfds`指定的描述符将被测试。    
	  注意`readfds`、`writefds`、`exceptfds`都是“值-结果”参数，调用select时其用于指定要监听的描述符，从select返回时内核会更改这些描述符集，指示哪些描述符已就绪。所以每次重新调用select函数时，都要再次把所有描述符内所关心的位均置1。    
	
  但这样改进后，对于将输入重定向到文件的批量输入仍存在问题：我们现在的处理是当从标准输入读取到EOF时，str_cli函数就此返回到main函数，而main函数随后终止。在批量输入方式下，标准输入中的EOF并不意味着我们同时也完成了从套接字的读入：可能仍有请求在去往服务器的路上，或者仍有应答在返回客户的路上。    
  客户进程将makefile重定向到输入，得到的回射输出少于输入文件：    
  ![tcpcli_RST_error](https://github.com/Wangzhike/mysocket/raw/master/myecho/picture/tcpcli_RST_error.png)    
  服务器子进程的str_echo函数read收到RST错误：    
  ![tcpserv_RST_error](https://github.com/Wangzhike/mysocket/raw/master/myecho/picture/tcpserv_RST_error.png)    

	
