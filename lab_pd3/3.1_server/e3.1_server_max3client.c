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

#define SOCKET int
#define LISTENQ 15	//# max coda delle richieste pendenti (cioè client in attesa di essere serviti)
#define MAXLEN 256
#define GETMSG "GET "
#define QUITMSG "QUIT\r\n"
#define ERRMSG "-ERR\r\n"
#define OKMSG "+OK\r\n"
#define MAXCHILDREN 3

char *prog_name;
pid_t parent_pid;
int nofchildren;

int readuntilLF(SOCKET s, char *ptr, size_t len);
ssize_t SendFile (int fd_out, int fd_in, void *vptr, size_t n);
void service(SOCKET connfd, struct sockaddr_in client_addr);
void sigint_handler();

int main (int argc, char *argv[]) {
	SOCKET listenfd, connfd;	//listenfd è relativo al socket passivo, connfd è relativo al socket connesso (cioè socket che deve fare accept)
	pid_t pid;
	uint16_t port;

	struct sockaddr_in saddr, caddr;
	socklen_t caddrlen;
	
	int i=0;
	nofchildren = 0;
	
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc != 2){
		err_quit("-usage: %s <IP address> <port>, where IP address must be in dotted decimal notation\n", prog_name);
	}else{
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
	}
	
	//inizializzo le strutture dati, altrimenti sia la bind che la accept danno errori dovuti alla memoria sporca
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	memset(&caddrlen, 0, sizeof(caddrlen));	//soprattutto questo va inizializzato
	
	//creo il socket passivo
	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listenfd < 0){
		err_quit("-socket() failed\n");
	}
	
	//specifico l'indirizzo su cui fare bind
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = port;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);	//ascolto da tutti gli indirizzi su una certa interfaccia
	
	printf("Binding on %s:%d\n",inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
	if(bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0){
		err_quit("-bind failed.\n");
	}
    
	Listen(listenfd, LISTENQ); //listen di sockwrap.c. Param: socket su cui mettersi in attesa, lungh max della coda dei client in attesa
	//gestisce da sola l'errore della listen
	
	printf("+socket %d is connected.\n", listenfd);
	
	parent_pid = getpid();
	
	/**	Quando attivo più di MAXCHILDREN, il server svolge lo stesso il 3-way handshake verso i quarto-e-oltre client
	 * 	ma i comandi che questi inviano non vengono processati dal server, perchè il socket non può ancora essere usato,
	 * 	dato che non è ancora stata fatta la accept.
	 * 	Se termino un client con CTRL-C il server rimane vivo e fa subito accept del primo client in attesa, processando
	 * 	anche tutti i comandi che quest'ultimo gli aveva inviato nel frattempo
	 * */
	
	while (1) {
		if(nofchildren >= MAXCHILDREN){
			printf("Cannot accept more connection. Server is busy.\n");
			wait(NULL);	//attendo che un figlio termini
			nofchildren--;	//un figlio è terminato, quindi nofchildren diminuisce
			continue;	//serve a saltare ciò che viene dopo questo scope e tornare al while
		}
		
		printf("\nWaiting for connections ...\n");

		int retry = 0;
		do {
			connfd = accept (listenfd, (SA*) &caddr, &caddrlen);
			if (connfd<0) {
				if (INTERRUPTED_BY_SIGNAL ||
				    errno == EPROTO || errno == ECONNABORTED ||
				    errno == EMFILE || errno == ENFILE ||
				    errno == ENOBUFS || errno == ENOMEM	) {
					retry = 1;
					err_ret ("-accept() failed");
				} else {
					err_ret ("-accept() failed");
					return 1;
				}
			} else {
				printf("+New connection from client %s:%u\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
				retry = 0;
			}
		} while (retry);
		
		if((pid = fork()) < 0){
			printf("Fork() failed.\n");
		}else if(pid > 0){	//padre
			close(connfd);
			nofchildren++;
		}else{	//figlio
			//signal(SIGUSR1, sigint_handler);
			close(listenfd);
			service(connfd, caddr);
			exit(0);	//altrimenti tornerei al main e questo figlio cercherebbe di fare accept di un socket chiuso
		}
	}
	
	while (wait(NULL) > 0){
		nofchildren--;
	};
	
	close(listenfd);
	printf("+socket has been closed.\n");
	exit(0);
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
	
	//continuo la lettura finchè non ricevo il quit dal client
	while(msgexit != 1){
		memset(buffer, 0, MAXLEN);	//metto a zero il buffer
		memset(filename, 0, MAXLEN);	//e anche la stringa che dovrà contenere il nome del file
		memset(filedescription, 0, 8);
		msglen = 0;
		msglen = readuntilLF(connfd, buffer, MAXLEN);
		if(msglen <= 0){
			printf("%d: -connection closed by %s\n", pid, (msglen==0)?"client":"server");
			close(connfd);
			break;
		}else{
			//grazie alla memset, non devo aggiungere '\0'. readuntilLF legge MAXLEN-1 caratteri, quindi l'ultimo è sempre \0
			if(strncasecmp(buffer, GETMSG, 4)==0 && buffer[strlen(buffer)-2] == '\r' && buffer[strlen(buffer)-1] == '\n'){	//la stringa è una "|G|E|T| |...|CR|LF|"
				printf("%d:>%s: %s\n", pid, inet_ntoa(caddr.sin_addr), buffer);
				sscanf(buffer, "GET %s\r\n", filename);
				fd = open(filename, O_RDONLY);
				if(fd < 0){
					printf("%d: %s\n", pid, ERRMSG);
					Send(connfd, ERRMSG, strlen(ERRMSG), MSG_NOSIGNAL);
				}else{
					fstat(fd, &stat_buf);
					filesize = stat_buf.st_size;
					modtime = stat_buf.st_mtim.tv_sec;
					printf("%d: filedes: %u, %u; sizeof(filesize)=%u, sizeof(modtime)=%u\n", pid, filesize, modtime, (uint)sizeof(filesize), (uint)sizeof(modtime));
					//trasformo filesize e modtime in network order
					fsize_bigendian = htonl(filesize);
					modtime = htonl(modtime);
					//printf("BIG ENDIAN filesize: %u\n", fsize_bigendian);
					//printf("sizeof(OKMSG) = %d\n", (int)sizeof(OKMSG));
				
					if((result = sendn(connfd, OKMSG, strlen(OKMSG), MSG_NOSIGNAL)) <= 0){
						if(result == 0)
							printf("%d: -Connection closed by client\n", pid);
						else
							printf("%d: -Error in sending file size\n", pid);
						break;	//esco dal while se sbaglio a mandare la dimensione
					}
					if((result = sendn(connfd, &fsize_bigendian, sizeof(uint32_t), MSG_NOSIGNAL)) <= 0){
						if(result == 0)
							printf("%d: -Connection closed by client\n", pid);
						else
							printf("%d: -Error in sending file size\n", pid);
						break;	//esco dal while se sbaglio a mandare la dimensione
					}
					if((result = sendn(connfd, &modtime, sizeof(uint32_t), MSG_NOSIGNAL)) <= 0){
						if(result == 0)
							printf("%d: -Connection closed by client\n", pid);
						else
							printf("%d: -Error in sending mod time\n", pid);
						close(fd);
						break;
					}
					if((result = SendFile(connfd, fd, 0, filesize)) < 0){
						printf("%d: -Error in sending file\n", pid);
						close(fd);
						break;
					}else{
						printf("%d: sent %d bytes\n", pid, result);
						close(fd);
					}
				}
			}
			else if(strcasecmp(buffer, QUITMSG)==0){
				printf("%d: >%s: %s\n", pid, inet_ntoa(caddr.sin_addr), QUITMSG);
				msgexit = 1;
			}
			else{	//caso di comando sconosciuto
				printf("%d: >%s: %s\n", pid, inet_ntoa(caddr.sin_addr), buffer);
				printf("%d: -Unknown command.\n", pid);
				if((result = sendn(connfd, ERRMSG, strlen(ERRMSG), MSG_NOSIGNAL)) <= 0){
					if(result == 0)
						printf("%d: -Connection closed by client\n", pid);
					else
						printf("%d: -Error in sending mod time\n", pid);
					close(fd);
					break;	
				}
			}
		}
	}
	close(connfd);
	printf("%d terminated\n", pid);
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
