/*
 * Sviluppato da Enrico Masala <masala@polito.it> , Mar 2016
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>


#include "errlib.h"
#include "sockwrap.h"


/* reads exactly "n" bytes from a descriptor, waiting a maximum amount of time */
ssize_t readn_timeout (int fd, void *vptr, size_t n,  struct timeval *p_tv) 
{
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		if (select(fd+1, &rset, NULL, NULL, p_tv) > 0) {
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
		} else {
			p_tv->tv_sec = -1;  // Set < 0 to detect timeout by caller
			p_tv->tv_usec = 0;
			return n - nleft; // timeout by caller is detected by checking the remaining time in p_tv
		}
	}
	return n - nleft;
}

/* reads a line (not beyond the \n), waiting a maximum amount of time */
ssize_t readline_unbuffered_timeout (int fd, void *vptr, size_t maxlen, struct timeval *p_tv)
{
	int n, rc;
	char c, *ptr;

	ptr = vptr;
	for (n=1; n<maxlen; n++)
	{
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		if (select(fd+1, &rset, NULL, NULL, p_tv) > 0) {
			if ( (rc = recv(fd,&c,1,0)) == 1)
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
		} else {
			p_tv->tv_sec = -1;  // Set < 0 to detect timeout by caller
			p_tv->tv_usec = 0;
			return n; // timeout by caller is detected by checking the remaining time in p_tv
		}
	}
	*ptr = 0; /* null terminate like fgets() */
	return n;
}


