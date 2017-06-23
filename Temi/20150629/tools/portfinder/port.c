#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <signal.h>
#include <string.h>

#include "../errlib.h"
#include "../sockwrap.h"

char *prog_name;

int connfd;

void close_all(int sig) 
{
	close(connfd);
	printf("CTRL-C detected - closing channel...\n");
	exit(0);
}

int main (int argc, char *argv[]) 
{
	int port, maxport;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen = sizeof(cliaddr);

	/* for errlib to know the program name */
	prog_name = argv[0];

	/* check arguments */
	if (argc != 1) 
		err_quit ("usage: %s", prog_name);	

	(void) signal (SIGINT, close_all);

	/* create socket */
	connfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* specify address to bind to */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	// We want the operating system to pick a random (available) port and this is the documented way to do that (call bind with port=0)
	servaddr.sin_port = 0;
	servaddr.sin_addr.s_addr = htonl (INADDR_ANY);

	if (bind(connfd, (SA*) &servaddr, sizeof(servaddr)) == 0) 
	{
		if (getsockname(connfd, (struct sockaddr *)&servaddr, &cliaddrlen) == -1)
		    perror("getsockname");
		else
		{
			printf("%d\n", ntohs(servaddr.sin_port));
			return ntohs(servaddr.sin_port);
		}
	} 
	else 
	{
		//printf("Error in bind()");
	}
	close(connfd);

	return 0;
}

