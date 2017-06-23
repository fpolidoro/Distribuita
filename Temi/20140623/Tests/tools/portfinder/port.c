/*
 *  warning: this is just a sample server to permit testing the clients; it is not as optimized or robust as it should be
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <rpc/xdr.h>

#include <signal.h>
#include <string.h>
#include <time.h>

#include "errlib.h"
#include "sockwrap.h"

//#define SRVPORT 1500

#define LISTENQ 15
#define MAXBUFL 255

#define MSG_ERR "wrong operands\r\n"
#define MSG_OVF "overflow\r\n"

#define MAX_UINT16T 0xffff
//#define STATE_OK 0x00
//#define STATE_V  0x01

#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif

char *prog_name;

int connfd;

void close_all(int sig) {
	close(connfd);
	printf("CTRL-C detected - closing channel...\n");
	//(void)signal(SIGINT, SIG_DFL);
	exit(1);
}

int main (int argc, char *argv[]) {
    
    
	int port, maxport;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen = sizeof(cliaddr);

	/* for errlib to know the program name */
	prog_name = argv[0];

	/* check arguments */
	if (argc != 2) 
		err_quit ("usage: %s <startport>", prog_name);
	sscanf(argv[1], "%d", &port);
	maxport = port+1000;
	

	(void) signal (SIGINT, close_all);

	/* create socket */
	connfd = socket(AF_INET, SOCK_STREAM, 0);

	while (port<maxport) {

		/* specify address to bind to */
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(port);
		servaddr.sin_addr.s_addr = htonl (INADDR_ANY);

		if (bind(connfd, (SA*) &servaddr, sizeof(servaddr)) == -1) {
			++port;
		} else {
			printf("%d", port);
			break;
		}
	}
	close(connfd);
	return 0;
}

