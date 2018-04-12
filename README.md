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


