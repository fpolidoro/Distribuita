#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "../errlib.h"
#include "../sockwrap.h"

#define SOCKET int
#define MAXLEN 256
#define STRLEN 250
#define GETMSG "GET "
#define QUITMSG "QUIT\r\n"
#define ERRMSG "-ERR\r\n"
#define OKMSG "+OK\r\n"
#define CHUNK_SIZE 1024*1024

char *prog_name;

int readuntilLF(SOCKET s, char *ptr, size_t len);
ssize_t RecvFile (int fd_out, SOCKET fd_in, size_t n);

int main (int argc, char *argv[]) {
	SOCKET s;
	int fd;
	int result = 0, quit = 0;
	uint16_t port;
	
	struct sockaddr_in saddr;
	socklen_t saddrlen;
	//inizializzo le due strutture
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	memset(&saddrlen, 0, sizeof(socklen_t));
	
	uint32_t modtime;	//tempo di ultima modifica del file
	uint32_t filesize;	//dimensione del file
	uint32_t fsize_bigendian;
	uint32_t modtime_bigendian;
	
	char *filename = NULL;
	char *nptr = NULL;
	char getmsg[MAXLEN+1];
	char okmsg[6];
	
	int i=0;
	uint32_t timestamp;
	uint32_t timestamp_bigendian;
	
	struct addrinfo hints, *res, *res0;	//puntatori per addrinfo, che ottiene le info sul server in base a se passo IPv4, IPv6 o nome
	
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc != 4){
		err_quit("usage: %s <IP address> <port> <filename>", prog_name);
	}else{		
		while(i<strlen(argv[2])){
		  if(!isdigit(argv[2][i]))
			 err_quit("port must be a number");
		  i++;
		}
		//converto la porta da stringa a intero short (atoi)
		port = atoi(argv[2]);
		if(port == 0 || port > 65535){
			err_quit("Port must be a number between 1024 and 65535");
		}
		
		if((filename = strdup(argv[3])) == NULL){
			err_quit("cannot allocate filename\n");
		}
	}
	//controllo che l'indirizzo del server sia raggiungibile
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	//param 1 e 2: indirizzo_server, porta o protocollo in formato stringa (per porta 3301 bisogna scrivere '"3301"', altrimenti ad es '"http"')
	if(getaddrinfo(argv[1], argv[2],&hints, &res0)){	//getaddrinfo ritorna != 0 se fallisce per qualche motivo
		err_quit("getaddrinfo() failed.\n");
	}
	
	for(res = res0; res != NULL; res = res->ai_next){	//controllo se il socket funziona provando ad usarlo: se non va, provo l'indirizzo successivo
		s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(s < 0){
			continue;
		}
		if(connect(s, res->ai_addr, res->ai_addrlen) < 0){
			close(s);
			s = -1; //il socket non è valido, visto che non riesco a connettermi
			continue;
		}
		break;
	}
	//qui il socket dovrebbe essere connesso
	freeaddrinfo(res0);
	if(s < 0){
		err_quit("Socket is not connected, maybe address or name were wrong\n");
		return s;
	}
	
	//printf("Socket %d is connected.\n", s);
	
	//printf("USAGE:\nGET request: just insert the name of the file, one per line\nQUIT message: press CTRL+D\n\n");
	
	memset(getmsg, 0, MAXLEN+1);
	nptr = NULL;
	
	//printf("§ sending GET message to %s\n", argv[1]);
	nptr = strchr(filename, '\n');
	if(nptr != NULL)
		*nptr = '\0';	//al char puntato da nptr assegno \0, cioè a \n sostituisco \0
	sprintf(getmsg, "%s%s\r\n", GETMSG, filename);
	/** NOTA: Quando invio messaggi di lunghezza non fissa e il server legge fino a \n, devo usare
	 * 	strlen(getmsg) invece che sizeof, altrimenti nel secondo caso, spedirei anche tutti gli \0
	 *  non usati, e al ciclo successivo di read del server, leggerei questi \0-schifezza gel getmsg
	 * */
	if((result = sendn(s, getmsg, strlen(getmsg), 0)) <= 0){
		/*if(result == 0)
			printf("§ Connection closed by server\n");
		else
			printf("§ Error in sending GET message\n");*/
		free(filename);
		return -1;
	}else{
		memset(okmsg, 0, 6);
		//printf("§ GET sent. Preparing for receiving file %s\n", filename);
		if((result = readuntilLF(s, okmsg, sizeof(OKMSG))) <= 0){
			//if(result == 0)	printf("§ Connection closed by server\n");
			//else printf("§ Error in receiving OK message from server\n");
			free(filename);
			return -1;
		}
		if(strcasecmp(okmsg, OKMSG)!= 0){
		//ho letto una schifezza, quindi chiudo la connessione
			if(strcasecmp(okmsg, ERRMSG) == 0){
				//printf("%s> -ERR\n", argv[1]);
				free(filename);
				return -1;
			}else{
				//printf("§ Server answered with an unknown message.\n§ Closing the communication.\n");
				free(filename);
				return -1;
			}
		}
		//vado avanti a leggere il resto
		if((result = readn(s, &fsize_bigendian, sizeof(uint32_t)))<=0){
			//if(result == 0)	printf("§ Connection closed by server\n");
			//else printf("§ Error in receiving file size from server\n");
			free(filename);
			return -1;
		}
		filesize = ntohl(fsize_bigendian);
		//printf("§ -Size: %33u\n", (uint)filesize);
		//controllo sulla dimensione del file (se è enorme potrebbe essere un DoS)
		if((result = readn(s, &modtime_bigendian, sizeof(uint32_t)))<=0){
			//if(result == 0)	printf("§ Connection closed by server\n");
			//else printf("§ Error in receiving mod time from server\n");
			free(filename);
			return -1;
		}
		modtime = ntohl(modtime_bigendian);
		//printf("§ -Last modification time: %18u\n", (uint)modtime);
		//apro il file richiesto
		fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		if (fd == -1) {
		  //fprintf(stderr, "Unable to create '%s'\n", filename);
			free(filename);
			return -1;
		}
		if((result = RecvFile(fd, s, filesize)) < 0){
			//printf("§ Error in receiving file '%s'. Aborting.\n", filename);
			close(fd);
			free(filename);
			return -1;
		}
		if((result = readn(s, &timestamp_bigendian, sizeof(uint32_t)))<=0){
			//if(result == 0)	printf("§ Connection closed by server\n");
			//else printf("§ Error in receiving timestamp from server\n");
			free(filename);
			return -1;
		}
		timestamp = ntohl(timestamp_bigendian);
		
		printf("TIMESTAMP %d\n", timestamp);
		
		//printf("§ File received correctly:\n");
		//printf("§ -Name: %33s\n§ -Size: %33u\n§ -Last modification time: %18u\n", filename, (uint)filesize, (uint)modtime);
		close(fd);
	}
	free(filename);
	close(s);
	//printf("§ Socket has been closed.\n");
	return 0;
}


int readuntilLF(SOCKET s, char *ptr, size_t len){	//equivalente di readline_unbuffered di sockwrap.c
	ssize_t nread;
	int n;
	char c;
	
	for(n=0; n<len; n++){
		if((nread = recv(s, &c, 1, 0)) ==1){	//ho letto correttamente un carattere
			*ptr++ = c; //aggiungo c al buffer e incremento la posizione del puntatore
			if(c == '\n'){
				break;
				}
		}else if(nread == 0){ //non ci sono dati da leggere
			if(n==0)
				return 0; //nessun dato è stato letto perchè c'è solo EOF
			else break; //qualcosa è stato letto in precedenza, ora raggiunta EOF
		}else{	//nread < 0, il socket è stato chiuso dalla controparte
			return -1;
			}
		}
	return n;	//ritorno il # di caratteri letti in totale
	//NOTA: questa stringa non termina con '\0', occorre aggiungerlo manualmente
}

/**
 * NOTA: da man sendfile(): The in_fd argument must correspond to a file which
 * supports mmap(2)-like operations (i.e., it cannot be a socket).
 * Perciò bisogna per forza usare read e write
 * */
ssize_t RecvFile (int fd_out, SOCKET fd_in, size_t n)
{
	size_t filesize;
	ssize_t nwritten, nread;
	char buffer[CHUNK_SIZE];
	
	filesize = n;
	
	while (filesize > 0)
	{
		if ((nread = recv(fd_in, buffer, (filesize > CHUNK_SIZE)? CHUNK_SIZE : filesize, MSG_NOSIGNAL)) < 0){
			//printf("§ Error in recv: %s\n", strerror(errno));
			return -1;
		}
		if((nwritten = write(fd_out, buffer, nread)) < 0){
			//printf("§ Error in writing data on file: %s\n", strerror(errno));
			return -1;
		}//else printf("§ %d bytes successfully written\n", (int)nwritten);

		filesize -= nwritten;		
	}
	
	return n;
}
