#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>

#include "../errlib.h"
#include "../sockwrap.h"
#include "../types.h"	//message XDR

#define SOCKET int
#define LISTENQ 15	//# max coda delle richieste pendenti (cioè client in attesa di essere serviti)
#define MAXLEN 256
#define GETMSG "GET "
#define QUITMSG "QUIT\r\n"
#define ERRMSG "-ERR\r\n"
#define OKMSG "+OK\r\n"
#define MAXCHILDREN 10
#define TWOMINS 20

char *prog_name;
pid_t parent_pid;
int nofchildren;
int sigpipe;

int readuntilLF(SOCKET s, char *ptr, size_t len);
ssize_t SendFile (int fd_out, int fd_in, void *vptr, size_t n);
void service(SOCKET connfd, struct sockaddr_in client_addr);
void service_XDR(SOCKET s, struct sockaddr_in client_addr);
void sigpipe_handler();

int main (int argc, char *argv[]) {
	SOCKET listenfd, connfd;	//listenfd è relativo al socket passivo, connfd è relativo al socket connesso (cioè socket che deve fare accept)
	pid_t pid;
	uint16_t port;

	struct sockaddr_in saddr, caddr;
	socklen_t caddrlen;
	
	int i=0;
	int is_xdr = 0;
	int max_children;
	int status;
	
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc < 3 || argc > 4){
		err_quit("-usage: %s [-x] <port> <max_children>\n*\t-x (optional) enables the XDR format of messages\n*\t0 < max_children <= %d\n*\n", prog_name, MAXCHILDREN);
	}else if(argc == 3){
		while(i<strlen(argv[1])){
		  if(!isdigit(argv[1][i]))
			 err_quit("-port must be a number\n");
		  i++;
		}
		//converto la porta da stringa a intero short (atoi)
		port = atoi(argv[1]);
		if(port == 0 || port > 65535){
			err_quit("-port must be a number between 1024 and 65535\n");
		}
		//converto la porta in big endian per poterla spedire in rete
		port = htons(port);
		
		//controllo che il argv[2] sia un numero tra 1 e 10 compresi, per avere il max # di figli in attesa di accept
		i=0;
		while(i<strlen(argv[2])){
		  if(!isdigit(argv[2][i]))
			 err_quit("%s is not a number\n", argv[2]);
		  i++;
		}
		max_children = atoi(argv[2]);
		if(max_children > MAXCHILDREN || max_children == 0){
			err_quit("%s must be a number between 0 and %d\n", MAXCHILDREN+1);
		}
	}else{	//ho il param -x
		if(strcasecmp(argv[1], "-x")!=0){
			err_quit("Unknown parameter %s\nAborting.\n", argv[1]);
		}
		is_xdr = 1;
		while(i<strlen(argv[2])){
		  if(!isdigit(argv[2][i]))
			 err_quit("-port must be a number\n");
		  i++;
		}
		//converto la porta da stringa a intero short (atoi)
		port = atoi(argv[2]);
		if(port == 0 || port > 65535){
			err_quit("-port must be a number between 1024 and 65535\n");
		}
		//converto la porta in big endian per poterla spedire in rete
		port = htons(port);
		
		//controllo che il argv[2] sia un numero tra 1 e 10 compresi, per avere il max # di figli in attesa di accept
		i=0;
		while(i<strlen(argv[3])){
		  if(!isdigit(argv[3][i]))
			 err_quit("%s is not a number\n", argv[3]);
		  i++;
		}
		max_children = atoi(argv[3]);
		if(max_children > MAXCHILDREN || max_children == 0){
			err_quit("%s must be a number between 0 and %d\n", MAXCHILDREN+1);
		}
		printf("....................................XDR mode....................................\n");
	}
	
	//inizializzo le strutture dati, altrimenti sia la bind che la accept danno errori dovuti alla memoria sporca
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	memset(&caddrlen, 0, sizeof(caddrlen));	//soprattutto questo va inizializzato
	
	//creo il socket passivo
	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listenfd < 0){
		err_quit("*\t-socket() failed\n");
	}
	
	//specifico l'indirizzo su cui fare bind
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = port;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);	//ascolto da tutti gli indirizzi su una certa interfaccia
	
	printf("*\tBinding on %s:%d\n",inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
	if(bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0){
		err_quit("*\t-bind failed.\n");
	}
    
	Listen(listenfd, LISTENQ); //listen di sockwrap.c. Param: socket su cui mettersi in attesa, lungh max della coda dei client in attesa
	//gestisce da sola l'errore della listen
	
	printf("*\t+socket %d is connected.\n", listenfd);
	
	parent_pid = getpid();
	printf("................................................................................\n");
	for(i=0; i<max_children; i++){
		if((pid = fork()) < 0){
			printf("Fork() failed.\n");
		}else if(pid == 0){	//figlio
			while (1) {
				sigpipe = 0;
				signal(sigpipe, sigpipe_handler);
				printf("\nC%d: Waiting for connections ...\n", getpid());

				int retry = 0;
				do {
					connfd = accept (listenfd, (SA*) &caddr, &caddrlen);
					if (connfd<0) {
						if (INTERRUPTED_BY_SIGNAL ||
							errno == EPROTO || errno == ECONNABORTED ||
							errno == EMFILE || errno == ENFILE ||
							errno == ENOBUFS || errno == ENOMEM	) {
							retry = 1;
							err_ret ("C%d -accept() failed", getpid());
						} else {
							err_ret ("C%d -accept() failed", getpid());
							return 1;
						}
					} else {
						printf("C%d +New connection from client %s:%u\n", getpid(), inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
						retry = 0;
					}
				} while (retry);
				
				if(is_xdr){	//ho scelto XDR
					printf("C%d: Siamo nell'XDR\n", getpid());
					service_XDR(connfd, caddr);
					printf("C%d: Uscito dal servizio XDR", getpid());
					close(connfd);
				}else{	//no param -x
					printf("C%d: Siamo nel semplice\n", getpid());
					service(connfd, caddr);
					close(connfd);
				}
			}
			close(listenfd);
			exit(0);
		}
	}
	//qui ci arriva solo il padre
	close(listenfd);
	while (wait(&status) > 0);	//attendo che tutti i figli abbiano terminato
	printf("P%d: All children have terminated.\n", getpid());
	
	exit(0);
}

void service_XDR(SOCKET s, struct sockaddr_in client_addr){
	struct sockaddr_in caddr;
	FILE *fd;
	int result;
	int msgexit = 0;	//booleano per uscire dal while della ricezione msg dal client
	char *buffer;
	char *filename;
	pid_t pid = getpid();
	
	struct stat stat_buf;	//in cui salvo la dimensione del file
	uint32_t modtime;	//tempo di ultima modifica del file
	uint32_t filesize;	//dimensione del file
	
	//per la select
	struct timeval tval;
	fd_set cset;
	int t = TWOMINS;
	
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
	//continuo la lettura finchè non ricevo il quit dal client
	while(msgexit != 1){
		memset(&res, 0, sizeof(message));
		memset(&req, 0, sizeof(message));
		
		FD_ZERO(&cset);
		FD_SET(s, &cset);
		tval.tv_sec = t;
		tval.tv_usec = 0;
		
		printf("C%d: Waiting for a command...\n", getpid());
		if((result = select(FD_SETSIZE, &cset, NULL, NULL, &tval)) < 0){
			printf("Select failed.\n");
			return;
		}
		if(result > 0){	
			if(!xdr_message(&xdrs_r, &req)){
				if(sigpipe){
					printf("C%d: Client closed the connection.\n", getpid());
					close(s);
					break;
				}
			}
			if(req.tag == GET){
				filename = strdup(req.message_u.filename);
				printf("C%d: GET %s\n", getpid(), filename);
				if((fd = fopen(filename, "r")) == NULL){
					printf("C%d: ERR Cannot find %s\n", pid, filename);
					res.tag = ERR;
					if(!xdr_message(&xdrs_w, &res)){
						if(sigpipe){
							printf("C%d: Client closed the connection.\n", getpid());
							close(s);
							break;
						}
					}
					break;
				}
				if(stat(filename, &stat_buf)) {
					printf("C%d: Impossible to stat requested file", getpid());
					
					res.tag = ERR;
					if(!xdr_message(&xdrs_w, &res)){
						if(sigpipe){
							printf("C%d: Client closed the connection.\n", getpid());
							close(s);
							break;
						}
					}
					break;
				}
				filesize = stat_buf.st_size;
				modtime = stat_buf.st_mtim.tv_sec;
				if((buffer = malloc(filesize * sizeof(char)))==NULL){
					printf("C%d: Not enough memory for sending the requested file.\n", getpid());
					fclose(fd);
					res.tag = ERR;
					if(!xdr_message(&xdrs_w, &res)){
						if(sigpipe){
							printf("C%d: Client closed the connection.\n", getpid());
							close(s);
							break;
						}
					}
					break;
				}
				fread(buffer, sizeof(char), filesize, fd);	//controllo correttezza omesso
				res.tag = OK;
				res.message_u.fdata.last_mod_time = modtime;
				res.message_u.fdata.contents.contents_len = filesize;
				res.message_u.fdata.contents.contents_val = buffer;
				if(!xdr_message(&xdrs_w, &res)){
					if(sigpipe){
						printf("C%d: Client closed the connection.\n", getpid());
						close(s);
						free(buffer);
						free(req.message_u.filename);	
						free(req.message_u.fdata.contents.contents_val);
						break;
					}	
				}
				printf("C%d: filesize: %u, %u; sizeof(filesize)=%u, sizeof(modtime)=%u\n", pid, filesize, modtime, (uint)sizeof(filesize), (uint)sizeof(modtime));
				free(buffer);
				free(req.message_u.filename);	
				free(req.message_u.fdata.contents.contents_val);
			}else if(req.tag == QUIT){
				printf("C%d: Received QUIT command. Closing connection with client\n", getpid());
				break;
			}else{	//caso di comando sconosciuto
				printf("C%d: >%s: %d\n", pid, inet_ntoa(caddr.sin_addr), req.tag);
				printf("C%d: -Unknown command.\n", pid);
				res.tag = ERR;
				if(!xdr_message(&xdrs_w, &res)){
					if(sigpipe){
						printf("Client closed the connection.\n");
						close(s);
						break;
					}
				}
			}
		}else{	//select: il timeout è scaduto
			printf("C%d: No activity from client since %d seconds. Closing connection.\n", pid, TWOMINS);
			break;
		}
	}
	
	xdr_destroy(&xdrs_w);
	xdr_destroy(&xdrs_r);
	fclose(stream_socket_r);
	fclose(stream_socket_w);
	return;
}

void service(SOCKET connfd, struct sockaddr_in client_addr){
	struct sockaddr_in caddr;
	int fd;
	int result;
	int msgexit = 0;	//booleano per uscire dal while della ricezione msg dal client
	int msglen;
	char buffer[MAXLEN];
	char filename[MAXLEN];
	char filedescription[8+1];
	pid_t pid = getpid();
	
	struct stat stat_buf;	//in cui salvo la dimensione del file
	uint32_t modtime;	//tempo di ultima modifica del file
	uint32_t filesize;	//dimensione del file
	uint32_t fsize_bigendian;
	
	//per la select
	struct timeval tval;
	fd_set cset;
	int t = TWOMINS;
	
	//continuo la lettura finchè non ricevo il quit dal client
	while(msgexit != 1){
		memset(buffer, 0, MAXLEN);	//metto a zero il buffer
		memset(filename, 0, MAXLEN);	//e anche la stringa che dovrà contenere il nome del file
		memset(filedescription, 0, 8);
		
		FD_ZERO(&cset);
		FD_SET(connfd, &cset);
		tval.tv_sec = t;
		tval.tv_usec = 0;
		
		if((result = select(FD_SETSIZE, &cset, NULL, NULL, &tval)) < 0){
			printf("Select failed.\n");
			return;
		}
		if(result > 0){
			msglen = 0;
			msglen = readuntilLF(connfd, buffer, MAXLEN);
			if(msglen <= 0){
				printf("C%d: -connection closed by %s\n", pid, (msglen==0)?"client":"server");
				close(connfd);
				break;
			}else{
				//grazie alla memset, non devo aggiungere '\0'. readuntilLF legge MAXLEN-1 caratteri, quindi l'ultimo è sempre \0
				if(strncasecmp(buffer, GETMSG, 4)==0 && buffer[strlen(buffer)-2] == '\r' && buffer[strlen(buffer)-1] == '\n'){	//la stringa è una "|G|E|T| |...|CR|LF|"
					printf("C%d:>%s: %s\n", pid, inet_ntoa(caddr.sin_addr), buffer);
					sscanf(buffer, "GET %s\r\n", filename);
					fd = open(filename, O_RDONLY);
					if(fd < 0){
						printf("C%d: %s\n", pid, ERRMSG);
						Send(connfd, ERRMSG, strlen(ERRMSG), MSG_NOSIGNAL);
					}else{
						fstat(fd, &stat_buf);
						filesize = stat_buf.st_size;
						modtime = stat_buf.st_mtim.tv_sec;
						printf("C%d: filedes: %u, %u; sizeof(filesize)=%u, sizeof(modtime)=%u\n", pid, filesize, modtime, (uint)sizeof(filesize), (uint)sizeof(modtime));
						//trasformo filesize e modtime in network order
						fsize_bigendian = htonl(filesize);
						modtime = htonl(modtime);
						//printf("BIG ENDIAN filesize: %u\n", fsize_bigendian);
						//printf("sizeof(OKMSG) = %d\n", (int)sizeof(OKMSG));
					
						if((result = sendn(connfd, OKMSG, strlen(OKMSG), MSG_NOSIGNAL)) <= 0){
							if(result == 0)
								printf("C%d: -Connection closed by client\n", pid);
							else
								printf("C%d: -Error in sending file size\n", pid);
							break;	//esco dal while se sbaglio a mandare la dimensione
						}
						if((result = sendn(connfd, &fsize_bigendian, sizeof(uint32_t), MSG_NOSIGNAL)) <= 0){
							if(result == 0)
								printf("C%d: -Connection closed by client\n", pid);
							else
								printf("C%d: -Error in sending file size\n", pid);
							break;	//esco dal while se sbaglio a mandare la dimensione
						}
						if((result = sendn(connfd, &modtime, sizeof(uint32_t), MSG_NOSIGNAL)) <= 0){
							if(result == 0)
								printf("C%d: -Connection closed by client\n", pid);
							else
								printf("C%d: -Error in sending mod time\n", pid);
							close(fd);
							break;
						}
						if((result = SendFile(connfd, fd, 0, filesize)) < 0){
							printf("C%d: -Error in sending file\n", pid);
							close(fd);
							break;
						}else{
							printf("C%d: sent %d bytes\n", pid, result);
							close(fd);
						}
					}
				}
				else if(strcasecmp(buffer, QUITMSG)==0){
					printf("C%d: >%s: %s\n", pid, inet_ntoa(caddr.sin_addr), QUITMSG);
					msgexit = 1;
				}
				else{	//caso di comando sconosciuto
					printf("C%d: >%s: %s\n", pid, inet_ntoa(caddr.sin_addr), buffer);
					printf("C%d: -Unknown command.\n", pid);
					if((result = sendn(connfd, ERRMSG, strlen(ERRMSG), MSG_NOSIGNAL)) <= 0){
						if(result == 0)
							printf("C%d: -Connection closed by client\n", pid);
						else
							printf("C%d: -Error in sending mod time\n", pid);
						close(fd);
						break;	
					}
				}
			}
		}else{
			printf("C%d: No activity from client since %d seconds. Closing connection.\n", pid, TWOMINS);
			break;
		}
	}
	printf("C%d terminated\n", pid);
	return;
}

/**	NOTA: il client in questo caso, deve sempre usare strlen(messaggio) quando fa la send, altrimenti,
 * 	se messaggio ha dimensione 256, contiene "ciao\n" e il resto è \0, scrivendo sizeof(messaggio),
 * 	spedirei ciao\n e anche tutti i \0 successivi. Di qui, quindi leggerei ciao\n al primo ciclo, ma
 * 	al secondo, la readuntilLF non sarebbe più bloccante perchè dovrebbe ancora leggere i \0 di prima.
 * 	Con strlen(messaggio) invece spedirei solamente ciao\n e basta (strlen non conta \0 come carattere).
 * */
int readuntilLF(SOCKET s, char *ptr, size_t len){	//equivalente di readline_unbuffered di sockwrap.c
	ssize_t nread;
	int n;
	char c;
	
	//leggo fino a len-1, così ho posto per mettere \0
	for(n=1; n<len-1; n++){
		if((nread = recv(s, &c, 1, MSG_NOSIGNAL)) == 1){	//ho letto correttamente un carattere, NOSIGNAL per evitare che il server crashi se il client viene chiuso
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

ssize_t SendFile (int fd_out, int fd_in, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	off_t *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ( (nwritten = sendfile(fd_out, fd_in, ptr, nleft)) < 0)
		{
			if (INTERRUPTED_BY_SIGNAL)
			{
				nwritten = 0;
				continue; /* and call send() again */
			}
			else
				return -1;
		}
		nleft -= nwritten;
		ptr   += nwritten;
		printf("%d bytes successfully written\n", (int)nwritten);
	}
	return n;
}
//nelle read/write&c posso fare il controllo if(sigpipe) then printf("il client ha chiuso la connessione"), ed uscire dal ciclo relativo a quella connessione
void sigpipe_handler(int signal) {
  printf(" %d - SIGPIPE captured!\n", getpid());
  sigpipe = 1;
  return;
}
