#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <rpc/xdr.h>

#include "../errlib.h"
#include "../sockwrap.h"
#include "../types.h"	//tipi xdr per i messaggi 

#define SOCKET int
#define MAXLEN 256
#define STRLEN 250
#define GETMSG "GET "
#define QUITMSG "QUIT\r\n"
#define ERRMSG "-ERR\r\n"
#define OKMSG "+OK\r\n"
#define AMSG "A\n"
#define QMSG "Q\n"
#define CHUNK_SIZE 1024*1024

char *prog_name;
int sigpipe;

void sigpipe_handler(int signal);
int readuntilLF(SOCKET s, char *ptr, size_t len);
ssize_t RecvFile (int fd_out, SOCKET fd_in, size_t n);
void service_XDR(SOCKET s);
void service(SOCKET s);

int main (int argc, char *argv[]) {
	SOCKET s;
	uint16_t port;
	
	struct sockaddr_in saddr;
	socklen_t saddrlen;
	//inizializzo le due strutture
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	memset(&saddrlen, 0, sizeof(socklen_t));
	
	//per poter usare XDR
	int is_xdr = 0;
	
	int i=0;
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc < 3 || argc > 4){
		//controllo il # di argomenti
		err_quit("USAGE: %s [-x] <IP address> <port>\n*\n*\t-x (optional) enables the XDR format of messages\n*\tIP address must be in dotted decimal notation\n*", prog_name);
	}else if(argc == 3){	//inserisco solo IP e porta
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
			err_quit("Port must be a number between 1024 and 65535");
		}
		//converto la porta in big endian per poterla spedire in rete
		port = htons(port);
	}else{	//ho inserito anche -x per dire che uso XDR
		if(strcasecmp(argv[1], "-x") == 0){
			is_xdr = 1;
		}else{
			err_quit("%s is not a valid parameter. Aborting\n", argv[1]);
		}
		if(strlen(argv[2]) > 16){
			err_quit("address is in wrong format");
		}		
		
		while(i<strlen(argv[3])){
		  if(!isdigit(argv[3][i]))
			 err_quit("port must be a number");
		  i++;
		}
		//converto la porta da stringa a intero short (atoi)
		port = atoi(argv[3]);
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
	if(is_xdr){
		if (inet_aton(argv[2], &saddr.sin_addr) == 0 ){
			perror(argv[2]);
			exit(errno);
		}
	}else{
		if (inet_aton(argv[1], &saddr.sin_addr) == 0 ){
			perror(argv[1]);
			exit(errno);
		}
	}
	
	sigpipe = 0;
	signal(SIGPIPE, sigpipe_handler);
	
	if(connect(s, (struct sockaddr *) &saddr, sizeof(saddr)) == -1){
		err_quit("Connect() failed");
	}
	printf("Socket %d is connected.\n", s);
	
	if(is_xdr){	//programma avviato con -x (XDR)
		service_XDR(s);
	}else{	//programma avviato senza XDR
		service(s);
	}
	
	close(s);
	printf("\n§ Socket has been closed.\n");
	return 0;
}

void service_XDR(SOCKET s){
	FILE *fd;
	int quit = 0;
	
	uint32_t modtime;	//tempo di ultima modifica del file
	uint32_t filesize;	//dimensione del file
	
	char string[STRLEN+1];
	char getmsg[MAXLEN+1];
	char *buffer;	//buffer che conterrà i byte del file (è un char*, vd types.h)
	char *nptr = NULL;
	
	//per poter usare XDR
	message req, res;
	XDR xdrs_r, xdrs_w;
	FILE *stream_socket_r;
	FILE *stream_socket_w;
	
	stream_socket_r = fdopen(s, "r");
	if (stream_socket_r == NULL)
		err_sys ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&xdrs_r, stream_socket_r, XDR_DECODE);

	stream_socket_w = fdopen(s, "w");
	if (stream_socket_w == NULL)
		err_sys ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&xdrs_w, stream_socket_w, XDR_ENCODE);
	
	setbuf(stream_socket_w, 0);	//per evitare la bufferizzazione, altrimenti scrivo il nome file per la GET e non spedisce niente

	//ciclo di lettura comandi da stdin
	while(!quit){
		memset(string, 0, STRLEN+1);	//metto a 0 la stringa, così non devo preoccuparmi di \0
		memset(getmsg, 0, MAXLEN+1);
		memset(&req, 0, sizeof(message));
		memset(&res, 0, sizeof(message));
		nptr = NULL;
		printf(" > ");
		/**
		 * NOTA: fgets acquisisce fino a MAXLEN caratteri: se MAXLEN=6 e io scrivo "ciaooo",
		 * la stringa non conterrà nè \n nè \0. Devo quindi pensare a riservare lo spazio sia per \r che \n che \0
		 * */
		if(fgets(string, STRLEN-2, stdin) == NULL){	//se scrivo e poi faccio CTRL+D, fgets non ritorna NULL. Lo ritorna solo se faccio CTRL+D senza scrivere nulla
			//EOF via CTRL+D
			printf("§ sending QUIT message to server\n");
			req.tag = QUIT;
			if(!xdr_message(&xdrs_w, &req)){
				if(sigpipe)
					printf("§ Connection closed by server\n");
				else
					printf("§ Error in sending QUIT message\n");
				break;
			}else
				quit = 1;	//esco dal ciclo in modo grazioso
		}else{
			printf("§ sending GET message to server\n");
			nptr = strchr(string, '\n');
			if(nptr != NULL)
				*nptr = '\0';	//al char puntato da nptr assegno \0, cioè a \n sostituisco \0
			req.tag = GET;
			req.message_u.filename = string;
			if(!xdr_message(&xdrs_w, &req)){
				if(sigpipe)
					printf("§ Connection closed by server\n");
				else
					printf("§ Error in sending GET message\n");
				break;
			}else{
				printf("§ GET sent. Preparing for receiving file %s\n", string);
				if(!xdr_message(&xdrs_r, &res)){
					if(sigpipe)	printf("§ Connection closed by server\n");
					else printf("§ Error in receiving OK message from server\n");
					break;
				}
				if(res.tag == ERR){
					printf("server> -ERR\n");
					break;
				}
				else if(res.tag == OK){
					//assegno i valori ricevuti da XDR
					filesize = res.message_u.fdata.contents.contents_len;
					modtime = res.message_u.fdata.last_mod_time;
					if((fd = fopen(string, "w"))==NULL){
						printf("§ Error in opening %s\nIgnoring file.\n", string);
						continue;	//continuo a richiedere un nuovo comando
					}
					buffer = res.message_u.fdata.contents.contents_val;
					if(fwrite(buffer, sizeof(char), filesize, fd)<filesize){
						printf("§ Error in saving file.\nAborting.\n");
						fclose(fd);
						continue;	//continuo a richiedere un nuovo comando
					}
					fclose(fd);
					printf("§ File received correctly:\n");
					printf("§ -Name: %33s\n§ -Size: %33u\n§ -Last modification time: %18u\n", string, (uint)filesize, (uint)modtime);
				}else{	//ho ricevuto un comando sconosciuto
					printf("§ Server answered with an unknown command.\n");
					continue;
				}
			}
		}
	}
	
	//distruggo le risorse occupate per gli xdr e chiudo i file descriptor relativi al socket
	xdr_destroy(&xdrs_w);
	xdr_destroy(&xdrs_r);
	fclose(stream_socket_r);
	fclose(stream_socket_w);
	return;	
	
}

void service(SOCKET s){
	int fd;
	int result = 0, quit = 0;
	
	uint32_t modtime;	//tempo di ultima modifica del file
	uint32_t filesize;	//dimensione del file
	uint32_t fsize_bigendian;
	uint32_t modtime_bigendian;
	
	char string[STRLEN+1];
	char getmsg[MAXLEN+1];		
	char *nptr = NULL;
	char okmsg[6];
	
	printf("\n................................................................................\n");
	printf("\n");
	printf("................................................................................\n\n");
	
		//ciclo di lettura comandi da stdin
	while(!quit){
		memset(string, 0, STRLEN+1);	//metto a 0 la stringa, così non devo preoccuparmi di \0
		memset(getmsg, 0, MAXLEN+1);
		nptr = NULL;
		printf(" > ");
		/**
		 * NOTA: fgets acquisisce fino a MAXLEN caratteri: se MAXLEN=6 e io scrivo "ciaooo",
		 * la stringa non conterrà nè \n nè \0. Devo quindi pensare a riservare lo spazio sia per \r che \n che \0
		 * */
		if(fgets(string, STRLEN-2, stdin) == NULL){	//se scrivo e poi faccio CTRL+D, fgets non ritorna NULL. Lo ritorna solo se faccio CTRL+D senza scrivere nulla
			//EOF via CTRL+D
			printf("§ sending QUIT message to server\n");
			
			if((result = sendn(s, QUITMSG, strlen(QUITMSG), 0)) <= 0){
				if(result == 0)
					printf("§ Connection closed by server\n");
				else
					printf("§ Error in sending QUIT message\n");
				break;
			}else
				quit = 1;	//esco dal ciclo in modo grazioso
		}else{
			printf("§ sending GET message to server\n");
			nptr = strchr(string, '\n');
			if(nptr != NULL)
				*nptr = '\0';	//al char puntato da nptr assegno \0, cioè a \n sostituisco \0
			sprintf(getmsg, "%s%s\r\n", GETMSG, string);
			/** NOTA: Quando invio messaggi di lunghezza non fissa e il server legge fino a \n, devo usare
			 * 	strlen(getmsg) invece che sizeof, altrimenti nel secondo caso, spedirei anche tutti gli \0
			 *  non usati, e al ciclo successivo di read del server, leggerei questi \0-schifezza gel getmsg
			 * */
			if((result = sendn(s, getmsg, strlen(getmsg), 0)) <= 0){
				if(result == 0)
					printf("§ Connection closed by server\n");
				else
					printf("§ Error in sending GET message\n");
				break;
			}else{
				memset(okmsg, 0, 6);
				printf("§ GET sent. Preparing for receiving file %s\n", string);
				if((result = readuntilLF(s, okmsg, sizeof(OKMSG))) <= 0){
					if(result == 0)	printf("§ Connection closed by server\n");
					else printf("§ Error in receiving OK message from server\n");
					break;
				}
				if(strcasecmp(okmsg, OKMSG)!= 0){
				//ho letto una schifezza, quindi chiudo la connessione
					if(strcasecmp(okmsg, ERRMSG) == 0){
						printf("server> -ERR\n");
						break;
					}else{
						printf("§ Server answered with an unknown message.\n§ Closing the communication.\n");
						break;
					}
				}
				//vado avanti a leggere il resto
				if((result = readn(s, &fsize_bigendian, sizeof(uint32_t)))<=0){
					if(result == 0)	printf("§ Connection closed by server\n");
					else printf("§ Error in receiving file size from server\n");
					break;
				}
				filesize = ntohl(fsize_bigendian);
				printf("§ -Size: %33u\n", (uint)filesize);
				//controllo sulla dimensione del file (se è enorme potrebbe essere un DoS)
				if((result = readn(s, &modtime_bigendian, sizeof(uint32_t)))<=0){
					if(result == 0)	printf("§ Connection closed by server\n");
					else printf("§ Error in receiving mod time from server\n");
					break;
				}
				modtime = ntohl(modtime_bigendian);
				printf("§ -Last modification time: %18u\n", (uint)modtime);
				//apro il file richiesto
				fd = open(string, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
				if (fd == -1) {
				  fprintf(stderr, "Unable to create '%s'\n", string);
				  break;
				}
				if((result = RecvFile(fd, s, filesize)) < 0){
					printf("§ Error in receiving file '%s'. Aborting.\n", string);
					close(fd);
					break;
				}
				printf("§ File received correctly:\n");
				printf("§ -Name: %33s\n§ -Size: %33u\n§ -Last modification time: %18u\n", string, (uint)filesize, (uint)modtime);
				close(fd);
			}
		}
	}
	return;
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
			printf("\n§ Error in recv: %s\n", strerror(errno));
			return -1;
		}
		if((nwritten = write(fd_out, buffer, nread)) < 0){
			printf("\n§ Error in writing data on file: %s\n", strerror(errno));
			return -1;
		}else printf("\n§ %d bytes successfully written\n", (int)nwritten);

		filesize -= nwritten;		
	}
	
	return n;
}

void sigpipe_handler(int signal) {
	if(signal == SIGPIPE){
		printf("SIGPIPE captured!\n");
		sigpipe = 1;
	}
	return;
}
