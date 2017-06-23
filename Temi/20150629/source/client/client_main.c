#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <utime.h>
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
#define GETMSG "GET"
#define QUITMSG "QUIT\r\n"
#define ERRMSG "-ERR\r\n"
#define OKMSG "+OK\r\n"
#define UPDMSG "UPD\r\n"
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
	
	char *filename;
	char *nptr = NULL;
	char getmsg[MAXLEN+1];
	char okmsg[6];
	
	struct utimbuf buf;
	
	int i=0;
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc != 4){
		//controllo che il # di argomenti non sia diverso da 2, che la prima stringa sia un indirizzo e che il terzo (partendo da 0), il settimo e l'undicesimo carattere siano un punto, e che il secondo parametro sia un numero
		err_quit("usage: %s <IP address> <port> <filename>, where IP address must be in dotted decimal notation", prog_name);
	}else{
		if(strlen(argv[1]) > 16 || strlen(argv[1]) < 7){
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
			err_quit("Port must be a number between 1024 and 65535");
		}
		//converto la porta in big endian per poterla spedire in rete
		port = htons(port);
	}
	
	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s < 0){
		err_quit("Socket() failed");
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
		err_quit("Connect() failed");
	}
	printf("Socket %d is connected.\n", s);
	
	printf("USAGE:\nGET request: just insert the name of the file, one per line\nQUIT message: press CTRL+D\n\n");
	
	memset(getmsg, 0, MAXLEN+1);
	nptr = NULL;
	printf(" > ");
	
	filename = strdup(argv[3]);
	assert(filename != NULL);
	
	printf("§ sending GET message to %s\n", argv[1]);
	nptr = strchr(filename, '\n');
	if(nptr != NULL)
		*nptr = '\0';	//al char puntato da nptr assegno \0, cioè a \n sostituisco \0
	sprintf(getmsg, "%s%s\r\n", GETMSG, filename);
	
	/** NOTA: Quando invio messaggi di lunghezza non fissa e il server legge fino a \n, devo usare
	 * 	strlen(getmsg) invece che sizeof, altrimenti nel secondo caso, spedirei anche tutti gli \0
	 *  non usati, e al ciclo successivo di read del server, leggerei questi \0-schifezza gel getmsg
	 * */
	if((result = sendn(s, getmsg, strlen(getmsg), 0)) <= 0){
		if(result == 0)
			printf("§ Connection closed by server\n");
		else
			printf("§ Error in sending GET message\n");
		free(filename);
		close(s);
		return -1;
	}else{
		memset(okmsg, 0, 6);
		printf("§GET sent. Waiting for file %s\n", filename);
		if((result = readuntilLF(s, okmsg, sizeof(OKMSG))) <= 0){
			if(result == 0)	printf("§ Connection closed by server\n");
			else printf("§ Error in receiving OK message from server\n");
			free(filename);
			close(s);
			return -1;
		}
		if(strcasecmp(okmsg, OKMSG)!= 0){
		//ho letto una schifezza, quindi chiudo la connessione
			if(strcasecmp(okmsg, ERRMSG) == 0){
				printf("%s> -ERR\n", argv[1]);
				free(filename);
				close(s);
				return -1;
			}else{
				printf("§ Server answered with an unknown message.\n§ Closing the communication.\n");
				free(filename);
				close(s);
				return -1;
			}
		}
		//vado avanti a leggere il resto
		//leggo il tempo di modifica
		if((result = readn(s, &modtime_bigendian, sizeof(uint32_t)))<=0){
			if(result == 0)	printf("§ Connection closed by server\n");
			else printf("§ Error in receiving mod time from server\n");
			free(filename);
			close(s);
			return -1;
		}
		modtime = ntohl(modtime_bigendian);
		printf("§ -Last modification time: %18u\n", (uint)modtime);
		
		//leggo la dimensione
		if((result = readn(s, &fsize_bigendian, sizeof(uint32_t)))<=0){
			if(result == 0)	printf("§ Connection closed by server\n");
			else printf("§ Error in receiving file size from server\n");
			free(filename);
			close(s);
			return -1;
			}
		filesize = ntohl(fsize_bigendian);
		printf("§ -Size: %33u\n", (uint)filesize);
		//controllo sulla dimensione del file (se è enorme potrebbe essere un DoS)
	
		//apro il file richiesto
		fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		if (fd == -1) {
			fprintf(stderr, "Unable to create '%s'\n", filename);
			free(filename);
			close(s);
			return -1;
		}
		if((result = RecvFile(fd, s, filesize)) < 0){
			printf("§ Error in receiving file '%s'. Aborting.\n", filename);
			close(fd);
			free(filename);
			close(s);
			return -1;
		}
		printf("§ File correctly received:\n");
		printf("§ -Name: %33s\n§ -Size: %33u\n§ -Last modification time: %18u\n", filename, (uint)filesize, (uint)modtime);
		close(fd);
		
		//imposto il tempo di modifica
		buf.actime = modtime;	//set access time
		buf.modtime = modtime;	//set mod time
		
		if (utime(filename, &buf)!=0) {
			// error
			printf("§ There was an error while setting modification time on %s.\n", filename);
			free(filename);
			close(s);
			return -1;
		}
		
		//ora mi metto in attesa degli aggiornamenti
		while(!quit){
			memset(okmsg, 0, 6);
			printf("§ Waiting for updates of %s\n", filename);
			if((result = readuntilLF(s, okmsg, sizeof(UPDMSG))) <= 0){
				if(result == 0)	printf("§ Connection closed by server\n");
				else printf("§ Error in receiving UPD message from server\n");
				free(filename);
				close(s);
				return -1;
			}
			if(strcasecmp(okmsg, UPDMSG)!= 0){
			//ho letto una schifezza, quindi chiudo la connessione
				if(strcasecmp(okmsg, ERRMSG) == 0){
					printf("%s> -ERR\n", argv[1]);
					free(filename);
					close(s);
					return -1;
				}else{
					printf("§ Server answered with an unknown message.\n§ Closing the communication.\n");
					free(filename);
					close(s);
					return -1;
				}
			}
			//vado avanti a leggere il resto
			//leggo il tempo di modifica
			if((result = readn(s, &modtime_bigendian, sizeof(uint32_t)))<=0){
				if(result == 0)	printf("§ Connection closed by server\n");
				else printf("§ Error in receiving mod time from server\n");
				free(filename);
				close(s);
				return -1;
			}
			modtime = ntohl(modtime_bigendian);
			printf("§ -Last modification time: %18u\n", (uint)modtime);
			
			//leggo la dimensione
			if((result = readn(s, &fsize_bigendian, sizeof(uint32_t)))<=0){
				if(result == 0)	printf("§ Connection closed by server\n");
				else printf("§ Error in receiving file size from server\n");
				free(filename);
				close(s);
				return -1;
			}
			filesize = ntohl(fsize_bigendian);
			printf("§ -Size: %33u\n", (uint)filesize);
			//controllo sulla dimensione del file (se è enorme potrebbe essere un DoS)
		
			//apro il file richiesto
			fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
			if (fd == -1) {
				fprintf(stderr, "Unable to create '%s'\n", filename);
				free(filename);
				close(s);
				return -1;
			}
			if((result = RecvFile(fd, s, filesize)) < 0){
				printf("§ Error in receiving file '%s'. Aborting.\n", filename);
				close(fd);
				free(filename);
				close(s);
				return -1;
			}
			printf("§ File correctly received:\n");
			printf("§ -Name: %33s\n§ -Size: %33u\n§ -Last modification time: %18u\n", filename, (uint)filesize, (uint)modtime);
			close(fd);
			
			//imposto il tempo di modifica
			//buf.actime = modtime;	//set access time
			buf.modtime = modtime;	//set mod time
			
			if (utime(filename, &buf)!=0) {
				// error
				printf("§ There was an error while setting modification time on %s.\n", filename);
				free(filename);
				close(s);
				return -1;
			}
		}
	}
	free(filename);
	close(s);
	printf("§ Socket has been closed.\n");
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
			printf("§ Error in recv: %s\n", strerror(errno));
			return -1;
		}
		if((nwritten = write(fd_out, buffer, nread)) < 0){
			printf("§ Error in writing data on file: %s\n", strerror(errno));
			return -1;
		}else printf("§ %d bytes successfully written\n", (int)nwritten);

		filesize -= nwritten;		
	}
	
	return n;
}
