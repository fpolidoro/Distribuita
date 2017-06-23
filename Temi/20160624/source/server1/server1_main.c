#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/time.h>

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

char *prog_name;

int readuntilLF(SOCKET s, char *ptr, size_t len);
ssize_t SendFile (int fd_out, int fd_in, void *vptr, size_t n);

int main (int argc, char *argv[]) {
	SOCKET listenfd, connfd;	//listenfd è relativo al socket passivo, connfd è relativo al socket connesso (cioè socket che deve fare accept)
	int fd;
	
	int result;
	int msgexit = 0;	//booleano per uscire dal while della ricezione msg dal client
	int msglen;
	uint16_t port;

	struct sockaddr_in saddr, caddr;
	socklen_t caddrlen;
	char buffer[MAXLEN];
	char filename[MAXLEN];
	char filedescription[8+1];
	
	struct stat stat_buf;	//in cui salvo la dimensione del file
	uint32_t modtime;	//tempo di ultima modifica del file
	uint32_t filesize;	//dimensione del file
	uint32_t fsize_bigendian;
	
	int i=0;
	//per salvare il tempo attuale
	struct timeval ts1, ts2;
	uint32_t ts2_ts1, tdiffn;
	
	
	//for errlib to know the name of the program
	prog_name = argv[0];
	if(argc != 2){
		err_quit("-usage: %s <port>\n", prog_name);
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
	//memset(&caddrlen, 0, sizeof(caddrlen));	//soprattutto questo va inizializzato
	// ^^ questo mi dà 0.0.0.0 quando stampo l'ip del client, meglio fare caddrlen = sizeof(caddr);
	
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
	
	//printf("Binding on %s:%d\n",inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
	if(bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0){
		err_quit("-bind failed.\n");
	}
    
	Listen(listenfd, LISTENQ); //listen di sockwrap.c. Param: socket su cui mettersi in attesa, lungh max della coda dei client in attesa
	//gestisce da sola l'errore della listen
	
	//printf("+socket %d is connected.\n", listenfd);
	
	while (1) {
		//printf("\nWaiting for connections ...\n");
		caddrlen = sizeof(caddr);	//se non faccio questo, quando stampo l'ip del client ottendo 0.0.0.0
		
		int retry = 0;
		do {
			connfd = 0;
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
				//printf("+New connection from client %s:%u\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
				retry = 0;
			}
		} while (retry);

		//continuo la lettura finchè non ricevo il quit dal client
		while(msgexit != 1){
			memset(buffer, 0, MAXLEN);	//metto a zero il buffer
			memset(filename, 0, MAXLEN);	//e anche la stringa che dovrà contenere il nome del file
			memset(filedescription, 0, 8);
			msglen = 0;
			msglen = readuntilLF(connfd, buffer, MAXLEN);
			if(msglen <= 0){
				//printf("-connection closed by %s\n", (msglen==0)?"client":"server");
				close(connfd);
				break;
			}else{
				//grazie alla memset, non devo aggiungere '\0'. readuntilLF legge MAXLEN-1 caratteri, quindi l'ultimo è sempre \0
				if(strncasecmp(buffer, GETMSG, 4)==0 && buffer[strlen(buffer)-2] == '\r' && buffer[strlen(buffer)-1] == '\n'){	//la stringa è una "|G|E|T| |...|CR|LF|"
					//prendo ts1
					if(gettimeofday(&ts1, NULL) != 0){
						//printf("-Error in getting ts1\n");
						break;
					}
					//printf(">%s: %s\n", inet_ntoa(caddr.sin_addr), buffer);
					sscanf(buffer, "GET %s\r\n", filename);
					fd = open(filename, O_RDONLY);
					if(fd < 0){
						//printf("%s\n", ERRMSG);
						Send(connfd, ERRMSG, strlen(ERRMSG), MSG_NOSIGNAL);
					}else{
						fstat(fd, &stat_buf);
						filesize = stat_buf.st_size;
						modtime = stat_buf.st_mtim.tv_sec;
						//printf("filedes: %u, %u; sizeof(filesize)=%u, sizeof(modtime)=%u\n", filesize, modtime, (uint)sizeof(filesize), (uint)sizeof(modtime));
						//trasformo filesize e modtime in network order
						fsize_bigendian = htonl(filesize);
						modtime = htonl(modtime);
						//printf("BIG ENDIAN filesize: %u\n", fsize_bigendian);
						//printf("sizeof(OKMSG) = %d\n", (int)sizeof(OKMSG));
						
						if((result = sendn(connfd, OKMSG, strlen(OKMSG), MSG_NOSIGNAL)) <= 0){
							/*if(result == 0)
								//printf("-Connection closed by client\n");
							else
								//printf("-Error in sending file size\n");*/
							break;	//esco dal while se sbaglio a mandare la dimensione
						}
						if((result = sendn(connfd, &fsize_bigendian, sizeof(uint32_t), MSG_NOSIGNAL)) <= 0){
							/*if(result == 0)
								//printf("-Connection closed by client\n");
							else
							//printf("-Error in sending file size\n");*/
							break;	//esco dal while se sbaglio a mandare la dimensione
						}
						if((result = sendn(connfd, &modtime, sizeof(uint32_t), MSG_NOSIGNAL)) <= 0){
							/*if(result == 0)
							//printf("-Connection closed by client\n");
							else
							//printf("-Error in sending mod time\n");*/
							close(fd);
							break;
						}
						if((result = SendFile(connfd, fd, 0, filesize)) < 0){
						//printf("-Error in sending file\n");
							close(fd);
							break;
						}else{
							//printf("sent %d bytes\n", result);
							close(fd);
						}
						//prendo ts2
						if(gettimeofday(&ts2, NULL) != 0){
						//printf("-Error in getting ts2\n");
							break;
						}
						ts2_ts1 = (ts2.tv_sec-ts1.tv_sec)*10000+(ts2.tv_usec-ts1.tv_usec);
						tdiffn = htonl(ts2_ts1);
						if((result = sendn(connfd, &tdiffn, sizeof(uint32_t), MSG_NOSIGNAL)) <= 0){
							/*if(result == 0)
							//printf("-Connection closed by client\n");
							else
							//printf("-Error in sending diff time\n");*/
							close(fd);
							break;
						}
						//<prefix> <client-ip> <client-port> <filename> <TS2-TS1>
						printf("OPERATION %s %d %s %d\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), filename, ts2_ts1);
						fflush(stdout);	//IMPORTANTE: altrimenti il test non dà esito positivo
					}
				}
				else if(strcasecmp(buffer, QUITMSG)==0){
				//printf(">%s: %s\n", inet_ntoa(caddr.sin_addr), QUITMSG);
					msgexit = 1;
				}
				else{	//caso di comando sconosciuto
				//printf(">%s: %s\n", inet_ntoa(caddr.sin_addr), buffer);
				//printf("-Unknown command.\n");
					if((result = sendn(connfd, ERRMSG, strlen(ERRMSG), MSG_NOSIGNAL)) <= 0){
						/*if(result == 0)
						//printf("-Connection closed by client\n");
						else
						//printf("-Error in sending mod time\n");*/
						close(fd);
						break;	
					}
				}
			}
		}
		close(connfd);
	}
	
	close(listenfd);
//printf("+socket has been closed.\n");
	return 0;
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
		//printf("%d bytes successfully written\n", (int)nwritten);
	}
	return n;
}
