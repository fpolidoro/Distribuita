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
#define MAXLEN 32

char *prog_name;


int main (int argc, char *argv[]) {
	SOCKET s;
	int result = 0;
	uint16_t port;
	
	char buffer[MAXLEN];
	
	struct sockaddr_in saddr, caddr;
	socklen_t caddrlen;
	
	int i=0;
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc != 2){
		//controllo che il # di argomenti non sia diverso da 2
		err_quit("usage: %s <port>\n", prog_name);
	}else{		
		while(i<strlen(argv[1])){	//controllo che la porta sia un numero
		  if(!isdigit(argv[1][i]))
			 err_quit("port must be a number");
		  i++;
		}
		//converto la porta da stringa a intero short (atoi)
		port = atoi(argv[1]);
		if(port == 0 || port > 65535){
			err_quit("port must be a number between 1024 and 65535");
		}
		//converto la porta in big endian per poterla spedire in rete
		port = htons(port);
	}
	
	//creo il socket (passivo)
	printf("Creating socket...");
	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s < 0){
		close(s);
		err_quit("socket() failed");
	}
	printf("DONE\n");
	
	//azzero la struct saddr
	bzero((char *) &saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = port;
   
	printf("Binding...");
    //faccio la bind
    if((result = bind(s, (struct sockaddr *)&saddr, sizeof(saddr))) < 0){
		close(s);
		err_quit("bind failed");
    }
    printf("DONE\n");
    while(1){
		printf("Cleaning memory...\n");
		memset(buffer, 0, MAXLEN + 1);
		printf("Waiting for a packet...\n");
		caddrlen = sizeof(caddr);	//altrimenti, se lo metto dopo la printf("Received a msg"), non funziona: al primo recv fa error nella sendTo, funziona dalla volta successiva
		//mi metto in ascolto di pacchetti in arrivo (recvFrom è bloccante). L'IP del mittente (caddr) è ottenuto dalla recvfrom in automatico
		result = recvfrom(s, buffer, MAXLEN, 0, (struct sockaddr *) &caddr, &caddrlen);
		if(result < 0){
			printf("Error in recvfrom\n");
		}else{
			printf("Received a message: %s \n", buffer);
			
			
			//leggo la risposta.
			result = sendto(s, buffer, result, 0, (struct sockaddr *)&caddr, caddrlen);
			if(result == -1)
				printf("Error in sendTo\n");
			else if(result < strlen(buffer))
				printf("non ho spedito tutti i byte\n");
			else
				printf("Message %s was correctly sent\n", buffer);
		}
	}
	
	close(s);
	return 0;
}
