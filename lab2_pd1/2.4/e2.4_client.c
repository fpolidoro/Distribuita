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
#include <rpc/xdr.h>

#include "../errlib.h"
#include "../sockwrap.h"

#define SOCKET int

char *prog_name;

int readuntilLF(SOCKET s, char *ptr, size_t len);

int main (int argc, char *argv[]) {
	SOCKET s;
	FILE *stream_socket_r, *stream_socket_w;
	int result = 0;
	uint16_t port;
	
	//numeri da spedire
	uint16_t short1, short2, res;
	XDR xdrs_r, xdrs_w;
	
	
	
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
	
	//collego il file descriptor s al FILE *stream_socket
	stream_socket_r = fdopen(s, "r");
	if (stream_socket_r == NULL)
		err_sys ("stream_socket_r error - fdopen() failed\n");
	xdrstdio_create(&xdrs_r, stream_socket_r, XDR_DECODE);

	stream_socket_w = fdopen(s, "w");
	if (stream_socket_w == NULL)
		err_sys ("stream_socket_w error - fdopen() failed\n");
	
	xdrstdio_create(&xdrs_w, stream_socket_w, XDR_ENCODE);
	xdrstdio_create(&xdrs_r, stream_socket_r, XDR_DECODE);
	
	
	printf("insert two unsigned integers: ");
	fscanf(stdin, "%hu %hu", &short1, &short2);
	
	//questi sono direttamente collegati al socket e fanno che spedire 
	//subito dopo la traduzione da uint del C a tipo dell'xdr
	if(!xdr_u_short(&xdrs_w, &short1)){
		printf("error while writing %u on server\n", short1);
	}else{
		fflush(stream_socket_w);
		printf("%u correctly written on server\n", short1);
	}
	if(!xdr_u_short(&xdrs_w, &short2)){
		printf("error while writing %u on server\n", short2);
	}else{
		fflush(stream_socket_w);
		printf("%u correctly written on server\n", short2);
	}
	
	printf("waiting for result...\n");
	
	if(!xdr_u_short(&xdrs_r, &res)){
		printf("error while reading result from server\n");
	}else{
		printf("result is %u\n", res);
	}
	
	
	xdr_destroy(&xdrs_r);
	xdr_destroy(&xdrs_w);
	fclose(stream_socket_w);
	fclose(stream_socket_r);
	close(s);
	printf("socket has been closed.\n");
	return 0;
}


int readuntilLF(SOCKET s, char *ptr, size_t len){	//equivalente di readline_unbuffered di sockwrap.c
	ssize_t nread;
	int n;
	char c;
	
	for(n=1; n<len; n++){
		if((nread = recv(s, &c, 1, 0)) ==1){	//ho letto correttamente un carattere
			*ptr++ = c; //aggiungo c al buffer e incremento la posizione del puntatore
			if(c == '\n'){
				break;
				}
		}else if(nread == 0){ //non ci sono dati da leggere
			if(n==1)
				return 0; //nessun dato è stato letto perchè c'è solo EOF
			else break; //qualcosa è stato letto in precedenza, ora raggiunta EOF
		}else{	//nread < 0, il socket è stato chiuso dalla controparte
			return -1;
			}
		}
	return n;	//ritorno il # di caratteri letti in totale
	//NOTA: questa stringa non termina con '\0', occorre aggiungerlo manualmente
}
