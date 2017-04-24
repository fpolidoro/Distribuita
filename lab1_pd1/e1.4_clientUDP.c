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
	int n;
	uint16_t port;
	
	char answer[50];
	char msg[32];
	
	//per la select
	struct timeval timeout;
	fd_set cset;
	timeout.tv_sec = 30; //imposto il # di secondi da attendere per timeout
	timeout.tv_usec = 0; //e 0 millisecondi
	
	struct sockaddr_in saddr;
	socklen_t saddrlen;
	
	int i=0;
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc != 4){
		//controllo che il # di argomenti non sia diverso da 4
		err_quit("usage: %s <IP address> <port> <msg>, where IP address must be in dotted decimal notation, and msg is a string of max 30 chars.\n", prog_name);
	}else{
		if(strlen(argv[1]) > 16){	//non ho scritto un indirizzo IP in notazione decimale puntata
			err_quit("address is in wrong format");
		}		
		
		while(i<strlen(argv[2])){	//controllo che la porta sia un numero
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
		
		//controllo che il msg sia di massimo 30 caratteri
		if(strlen(argv[3]) > 31){
			err_quit("Msg must be at most 30 chars long\n");
		}else
			strcpy(msg, argv[3]);
	}
	
	//creo il socket
	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s < 0){
		err_quit("socket() failed");
	}
	
	//imposto il socket s su fd, per poter usare la select
	FD_ZERO(&cset);	//azzero la struct cset
	FD_SET(s, &cset);	//le assegno il socket s
	
	saddr.sin_family = AF_INET;
	saddr.sin_port = port;
	if ( inet_aton(argv[1], &saddr.sin_addr) == 0 )	//converto l'indirizzo IP in big endian
   {
       perror(argv[1]);
       exit(errno);
   }
   printf("Send packet to: %s: %s\n", argv[1], argv[2]);
   //spedisco il pacchetto all'indirizzo specificato
   /**		NOTA: se faccio sendTo su una porta su cui il server non sta ascoltando oppure
    * 		se la faccio verso un indirizzo inesistente (es. 10.0.0.1, su una rete locale 192.168.1.0/24),
    * 		la sendTo va a buon fine, cioè spedisce il pacchetto, senza curarsi se sia arrivato o meno.
    * 		Guardando con Wireshark, vedo il pacchetto UDP verso la destinazione, ma ricevo un ICMP
    * 		destination unreachable nel caso di porta sbagliata, e niente nel caso di indirizzo inesistente
    * **/
   result = sendto(s, msg, strlen(msg), 0, (struct sockaddr *) &saddr, sizeof(saddr));
   if(result < strlen(msg)){
		//non ho inviato tutti i byte
		printf("non ho spedito tutti i byte");
	}
	else{
		printf("Message correctly sent. Waiting for the anwser.\n");
		//la select si sveglia solo quando scade il timeout, non mi interessano nè la scrittura nè l'eccezione
		if((n = select(FD_SETSIZE, &cset, NULL, NULL, &timeout))== -1){
			perror("select() failed.\n");	//la select ha ritornato n<0 => errore
		}
		if(n > 0){	//la select è stata svegliata dal server, che ha inviato la risposta
			saddrlen = sizeof(saddr);
			//leggo la risposta
			result = recvfrom(s, answer, strlen(msg), 0, (struct sockaddr *)&saddr, &saddrlen);
			if(n != -1)
				printf("answer: %s\n", answer);
			else
				printf("Error in receiving response\n");
		}else{ //select ritorna == 0: è scaduto il timeout
			printf("No response after 30 seconds\n");
		}
	}
	
	close(s);
	return 0;
}
