#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>

#include "errlib.h"
#include "sockwrap.h"

#define SOCKET int

char *prog_name;

int main (int argc, char *argv[]) {
	SOCKET s;
	int result = 0;
	uint16_t port;
	
	//numeri da spedire
	uint16_t short1, short2;
	char *num1, *num2;
	
	struct sockaddr_in saddr;
	
	int i=0;
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc != 3){
		//controllo che il # di argomenti non sia diverso da 2, che la prima stringa sia un indirizzo e che il terzo (partendo da 0), il settimo e l'undicesimo carattere siano un punto, e che il secondo parametro sia un numero
		err_quit("usage: %s <IP address> <port>, where IP address must be in dotted decimal notation", prog_name);
	}else{
		if(strlen(argv[1]) > 16){
			err_quit("address is in wrong format");
		}		
		
		while(i<strlen(argv[2])){
		  if(!isdigit(argv[2][i]))
			 err_quit("port must be a number");
		  i++;
		}
		//converto la porta da stringa a intero short (atoi)
		port = atoi(argv[2]);
		if(port == 0 || port > 65535){
			err_quit("port must be a number between 1024 and 65535");
		}
		//converto la porta in big endian per poterla spedire in rete
		port = htons(port);
	}
	
	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s < 0){
		err_quit("socket() failed");
	}
	
	saddr.sin_family = AF_INET;
	saddr.sin_port = port;
	if ( inet_aton(argv[1], &saddr.sin_addr) == 0 )
   {
       perror(argv[1]);
       exit(errno);
   }
    
	result = connect(s, (struct sockaddr *) &saddr, sizeof(saddr));
	if(result == -1){
		err_quit("connect() failed");
	}
	printf("socket %d is connected.\n", s);
	
	printf("insert two unsigned integers: ");
	fscanf(stdin, "%u %u", &short1, &short2);
	
	sprintf(num1, "%u", short1);
	sprintf(num2, "%u", short2);
	
	close(s);
	printf("socket has been closed.\n");
	return 0;
}