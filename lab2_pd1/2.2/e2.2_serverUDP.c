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

#include "../errlib.h"
#include "../sockwrap.h"

#define SOCKET int
#define MAXLEN 32
#define TRUE 1
#define FALSE 0
#define MAXREQ 3
#define NOFCLIENTS 10

char *prog_name;

typedef struct{
	char *address;	//ip del client che invia la richiesta al server
	int count;	//# di richieste inviate al server dal client
}client;


int main (int argc, char *argv[]) {
	SOCKET s;
	int result = 0;
	uint16_t port;
	
	char buffer[MAXLEN];
	
	struct sockaddr_in saddr, caddr;
	client client_list[NOFCLIENTS];
	socklen_t caddrlen;
	
	int i=0, j=0;
	int index = 0;
	int found = FALSE;
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
    
    for(i=0; i<NOFCLIENTS; i++){
		client_list[i].address = NULL;
		client_list[i].count = 0;
	}
    
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
			printf("Received a message: %s from %s\n", buffer, inet_ntoa(caddr.sin_addr));
			for(i=0; i<NOFCLIENTS; i++){
				if(client_list[i].address != NULL && strcmp(client_list[i].address, inet_ntoa(caddr.sin_addr))==0){
					client_list[i].count++;	     //aggiungo +1 al # richieste del client, se esiste
					found = TRUE;
					index = i;	//indice del client all'interno della lista dei client
					printf("client %s found at position %d of the array, with counter %d\n", client_list[i].address, i,client_list[i].count);
					break;
				}
			}
			if(found == TRUE){	//riporto a false, per poter fare il ciclo alla prossima recv
				found = FALSE;
			}else{	//il client non esisteva ancora in lista, lo aggiungo alla posizione j della lista
				if(j == NOFCLIENTS)
					j = j%NOFCLIENTS;	//azzero j, se la lista è già piena
				if(client_list[j].address != NULL){
					free(client_list[j].address);
					client_list[j].address = NULL;				
				}
				client_list[j].address = strdup(inet_ntoa(caddr.sin_addr));
				client_list[j].count = 0;
				index = 0;
				printf("client %s added to position %d of the array, with counter %d\n", client_list[0].address, j, client_list[0].count);
				j++;
			}
			
			//controllo che il client non abbia fatto più di 3 richieste e se ok, invio la risposta
			if(client_list[index].count < MAXREQ){
				//leggo la risposta.
				result = sendto(s, buffer, result, 0, (struct sockaddr *)&caddr, caddrlen);
				if(result == -1)
					printf("Error in sendTo\n");
				else if(result < strlen(buffer))
					printf("non ho spedito tutti i byte\n");
				else
					printf("Message %s was correctly sent\n", buffer);
			}else{
				printf("client %s sent more than 3 requests. Server is not going to answer.\n", client_list[index].address);
			}
		}
	}
	
	close(s);
	return 0;
}
