#ifndef _SOCKWRAP_H

#define _SOCKWRAP_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SA struct sockaddr

int Socket (int family, int type, int protocol);

void Bind (int sockfd, const SA *myaddr, socklen_t myaddrlen);

void Listen (int sockfd, int backlog);

int Accept (int listen_sockfd, SA *cliaddr, socklen_t *addrlenp);

void Connect (int sockfd, const SA *srvaddr, socklen_t addrlen);

void Close (int fd);

void Shutdown (int fd, int howto);

ssize_t Read (int fd, void *bufptr, size_t nbytes);

void Write (int fd, void *bufptr, size_t nbytes);

ssize_t Recv (int fd, void *bufptr, size_t nbytes, int flags);

ssize_t Recvfrom (int fd, void *bufptr, size_t nbytes, int flags, SA *sa, socklen_t *salenptr);

ssize_t Recvfrom_timeout (int fd, void *bufptr, size_t nbytes, int flags, SA *sa, socklen_t *salenptr, int timeout);

void Sendto (int fd, void *bufptr, size_t nbytes, int flags, const SA *sa, socklen_t salen);

void Send (int fd, void *bufptr, size_t nbytes, int flags);

/*
 * deprecated and no more supported by recent systems
 * 

void Inet_aton (const char *strptr, struct in_addr *addrptr);

*/

void Inet_pton (int af, const char *strptr, void *addrptr);

void Inet_ntop (int af, const void *addrptr, char *strptr, size_t length);

ssize_t readn (int fd, void *vptr, size_t n);

ssize_t Readn (int fd, void *ptr, size_t nbytes);

ssize_t readline (int fd, void *vptr, size_t maxlen);

ssize_t Readline (int fd, void *ptr, size_t maxlen);

ssize_t writen(int fd, const void *vptr, size_t n);

void Writen (int fd, void *ptr, size_t nbytes);

int Select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);

pid_t Fork (void);

struct hostent *Gethostbyname (const char *hostname);

void Getsockname (int sockfd, struct sockaddr *localaddr, socklen_t *addrp);

void Getaddrinfo ( const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

#endif
