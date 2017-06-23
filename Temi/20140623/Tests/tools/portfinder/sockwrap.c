#include <stdlib.h> // getenv()
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> // timeval
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_aton()
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include "errlib.h"
#include "sockwrap.h"

#define INTERRUPTED_BY_SIGNAL (errno == EINTR || errno == ECHILD)

extern char *prog_name;

int Socket (int family, int type, int protocol)
{
	int n;
	if ( (n = socket(family,type,protocol)) < 0)
		err_sys ("(%s) error - socket() failed", prog_name);
	return n;
}

void Bind (int sockfd, const SA *myaddr,  socklen_t myaddrlen)
{
	if ( bind(sockfd, myaddr, myaddrlen) != 0)
		err_sys ("(%s) error - bind() failed", prog_name);
}

void Listen (int sockfd, int backlog)
{
	char *ptr;
	if ( (ptr = getenv("LISTENQ")) != NULL)
		backlog = atoi(ptr);
	if ( listen(sockfd,backlog) < 0 )
		err_sys ("(%s) error - listen() failed", prog_name);
}


int Accept (int listen_sockfd, SA *cliaddr, socklen_t *addrlenp)
{
	int n;
again:
	if ( (n = accept(listen_sockfd, cliaddr, addrlenp)) < 0)
	{
		if (INTERRUPTED_BY_SIGNAL)
			goto again;
		else
			err_sys ("(%s) error - accept() failed", prog_name);
	}
	return n;
}


void Connect (int sockfd, const SA *srvaddr, socklen_t addrlen)
{
	if ( connect(sockfd, srvaddr, addrlen) != 0)
		err_sys ("(%s) error - connect() failed", prog_name);
}


void Close (int fd)
{
	if (close(fd) != 0)
		err_sys ("(%s) error - close() failed", prog_name);
}


void Shutdown (int fd, int howto)
{
	if (shutdown(fd,howto) != 0)
		err_sys ("(%s) error - shutdown() failed", prog_name);
}


ssize_t Read (int fd, void *bufptr, size_t nbytes)
{
	ssize_t n;
again:
	if ( (n = read(fd,bufptr,nbytes)) < 0)
	{
		if (INTERRUPTED_BY_SIGNAL)
			goto again;
		else
			err_sys ("(%s) error - read() failed", prog_name);
	}
	return n;
}


void Write (int fd, void *bufptr, size_t nbytes)
{
	if (write(fd,bufptr,nbytes) != nbytes)
		err_sys ("(%s) error - write() failed", prog_name);
}

ssize_t Recv(int fd, void *bufptr, size_t nbytes, int flags)
{
	ssize_t n;

	if ( (n = recv(fd,bufptr,nbytes,flags)) < 0)
		err_sys ("(%s) error - recv() failed", prog_name);
	return n;
}

ssize_t Recvfrom (int fd, void *bufptr, size_t nbytes, int flags, SA *sa, socklen_t *salenptr)
{
	ssize_t n;

	if ( (n = recvfrom(fd,bufptr,nbytes,flags,sa,salenptr)) < 0)
		err_sys ("(%s) error - recvfrom() failed", prog_name);
	return n;
}


ssize_t Recvfrom_timeout (int fd, void *bufptr, size_t nbytes, int flags, SA *sa, socklen_t *salenptr, int timeout)
{
	ssize_t n;
again:
	if ( (n = recvfrom(fd,bufptr,nbytes,flags,sa,salenptr)) < 0)
	{
		if (INTERRUPTED_BY_SIGNAL)
		{
			if (errno==EINTR && timeout)
				return -1;
			else
				goto again;
		}
		else
			err_sys ("(%s) error - recvfrom() failed", prog_name);
	}
	return n;
}


void Sendto (int fd, void *bufptr, size_t nbytes, int flags, const SA *sa, socklen_t salen)
{
	if (sendto(fd,bufptr,nbytes,flags,sa,salen) != nbytes)
		err_sys ("(%s) error - sendto() failed", prog_name);
}

void Send (int fd, void *bufptr, size_t nbytes, int flags)
{
	if (send(fd,bufptr,nbytes,flags) != nbytes)
		err_sys ("(%s) error - send() failed", prog_name);
}

/*
 * deprecated and no more supported in recent systems
 *

 void Inet_aton (const char *strptr, struct in_addr *addrptr)
 {
 if (inet_aton(strptr, addrptr) == 0)
 err_quit ("(%s) error - inet_aton() failed for '%s'", prog_name, strptr);
 }

 */

void Inet_pton (int af, const char *strptr, void *addrptr)
{
	int status = inet_pton(af, strptr, addrptr);
	if (status == 0)
		err_quit ("(%s) error - inet_pton() failed for '%s': invalid address", prog_name, strptr);
	if (status < 0)
		err_sys ("(%s) error - inet_pton() failed for '%s'", prog_name, strptr);
}


void Inet_ntop (int af, const void *addrptr, char *strptr, size_t length)
{
	if ( inet_ntop(af, addrptr, strptr, length) == NULL)
		err_quit ("(%s) error - inet_ntop() failed: invalid address", prog_name);
}


#ifndef MAXLINE
#define MAXLINE 1024
#endif

/* reads exactly "n" bytes from a descriptor */

ssize_t readn (int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ( (nread = read(fd, ptr, nleft)) < 0)
		{
			if (INTERRUPTED_BY_SIGNAL)
			{
				nread = 0;
				continue; /* and call read() again */
			}
			else
				return -1;
		}
		else
			if (nread == 0)
				break; /* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return n - nleft;
}


ssize_t Readn (int fd, void *ptr, size_t nbytes)
{
	ssize_t n;

	if ( (n = readn(fd, ptr, nbytes)) < 0)
		err_sys ("(%s) error - readn() failed", prog_name);
	return n;
}


/*

   read a whole buffer, for performance, and then return one char at a time

 */
static ssize_t my_read (int fd, char *ptr)
{
	static int read_cnt = 0;
	static char *read_ptr;
	static char read_buf[MAXLINE];

	if (read_cnt <= 0)
	{
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0)
		{
			if (INTERRUPTED_BY_SIGNAL)
				goto again;
			return -1;
		}
		else
			if (read_cnt == 0)
				return 0;
		read_ptr = read_buf;
	}
	read_cnt--;
	*ptr = *read_ptr++;
	return 1;
}


ssize_t readline (int fd, void *vptr, size_t maxlen)
{
	int n, rc;
	char c, *ptr;

	ptr = vptr;
	for (n=1; n<maxlen; n++)
	{
		if ( (rc = my_read(fd,&c)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		}
		else if (rc == 0)
		{
			if (n == 1)
				return 0; /* EOF, no data read */
			else
				break; /* EOF, some data was read */
		}
		else
			return -1; /* error, errno set by read() */
	}
	*ptr = 0; /* null terminate like fgets() */
	return n;
}


ssize_t Readline (int fd, void *ptr, size_t maxlen)
{
	ssize_t n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		err_sys ("(%s) error - readline() failed", prog_name);
	return n;
}

ssize_t writen (int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ( (nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (INTERRUPTED_BY_SIGNAL)
			{
				nwritten = 0;
				continue; /* and call write() again */
			}
			else
				return -1;
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return n;
}


void Writen (int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		err_sys ("(%s) error - writen() failed", prog_name);
}


int Select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout)
{
	int n;
again:
	if ( (n = select (maxfdp1, readset, writeset, exceptset, timeout)) < 0)
	{
		if (INTERRUPTED_BY_SIGNAL)
			goto again;
		else
			err_sys ("(%s) error - select() failed", prog_name);
	}
	return n;
}


pid_t Fork (void)
{
	pid_t pid;
	if ((pid=fork()) < 0)
		err_sys ("(%s) error - fork() failed", prog_name);
	return pid;
}

#ifdef SOLARIS
const char * hstrerror (int err)
{
	if (err == NETDB_INTERNAL)
		return "internal error - see errno";
	if (err == NETDB_SUCCESS)
		return "no error";
	if (err == HOST_NOT_FOUND)
		return "unknown host";
	if (err == TRY_AGAIN)
		return "hostname lookup failure";
	if (err == NO_RECOVERY)
		return "unknown server error";
	if (err == NO_DATA)
		return "no address associated with name";
	return "unknown error";
}
#endif

struct hostent *Gethostbyname (const char *hostname)
{
	struct hostent *hp;
	if ((hp = gethostbyname(hostname)) == NULL)
		err_quit ("(%s) error - gethostbyname() failed for '%s': %s",
				prog_name, hostname, hstrerror(h_errno));
	return hp;
}

void Getsockname (int sockfd, struct sockaddr *localaddr, socklen_t *addrp)
{
	if ((getsockname(sockfd, localaddr, addrp)) != 0)
		err_quit ("(%s) error - getsockname() failed", prog_name);
}

void Getaddrinfo ( const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
	int err_code;
	err_code = getaddrinfo(node, service, hints, res);
	if (err_code!=0) {
		err_quit ("(%s) error - getaddrinfo() failed  %s %s : %s", prog_name, node, service, gai_strerror(err_code));
	}
}

