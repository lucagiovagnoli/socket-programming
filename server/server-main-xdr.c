/*
 ============================================================================
 Name        : socket_main.c
 Author      : Luca Giovagnoli
 Version     : DEBUG
 Copyright   : Your copyright notice
 Description : server dell'esame PD1 17/07/2013, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <wait.h>

#include <rpc/xdr.h>

#include "wrapsock_mod.h"
#include "types.h"
#include "my_transfer_n.h"

#define MAXLINEE 1000
#define BUF 8192
#define LINE 50
#define NCHILDREN 5

/* Puoi definire DEBUG in fase di compilazione utilizzando l'opzione
 * -D di gcc (es. -DDEBUG=10). Nel caso sia omesso il valore (-DDEBUG)
 * allora il defaul è -DDEBUG=1 (vedi "man gcc").
 * Ovviamente l'espansione di __VA_ARGS__ consiste in tutti gli
 * argomenti indicati dai tre puntini.*/

#ifdef DEBUG
#define debug_fprintf(...) fprintf(stderr,__VA_ARGS__)
#else
#define debug_fprintf(...)
#endif

typedef struct vettore{
	int n;
	float vettore[MAXLINEE];
}VETTORE;

void input_file(VETTORE* numeri, char* filename);
void socket_setup(int* listen_sock,char* porta);
void ricevi_dati(VETTORE vettore_s,int sd);
void invia_risposta(int sd, bool_t esito,float risultato);
int codifica_xdr(char* buffer, Response* blocco);
bool_t decodifica_xdr(FILE* sock_p,Request* blocco);
int my_signal(int signo,void (*funzione)(int signo));
void attendi_figli(int signo);
void termina_figli();


static int n_figli=0;
static char *prog_name;
static int figli[NCHILDREN];

int main(int argc, char* argv[]) {

	VETTORE vettore_s;
	int listen_sock, accepted_sd;
	struct sockaddr_in addr_sorgente;
	socklen_t dim_client_addr;
	pid_t pid_figlio;

	if(argc!=3){
		debug_fprintf("(%s)(parent) Numero di argomenti errato\n",prog_name);
		exit(-1);
	}

	prog_name = argv[0];

	input_file(&vettore_s,argv[2]);
	socket_setup(&listen_sock,argv[1]);

	my_signal(SIGCHLD,attendi_figli);
	my_signal(SIGINT,termina_figli);

	while(1){
		/*accept è una funzione value-result quindi ottengo in ritorno l'indirizzo di chi si è connesso*/
		dim_client_addr= sizeof(addr_sorgente);

		while(n_figli>=NCHILDREN){
			pause();
		}

		while((accepted_sd=accept(listen_sock,(struct sockaddr*) &addr_sorgente, &dim_client_addr))<0){
			if(errno==EINTR) {
				debug_fprintf("(%s)(parent) System call accept interrotta da un segnale (errno = EINTR). Let's continue\n",prog_name);
				continue;
			}
			else {
				perror("Errore nella accept");
				exit(-1);
			}
		}

		if((pid_figlio = fork())==0){
			/*processo figlio*/
			my_signal(SIGINT,SIG_DFL);
			my_signal(SIGCHLD,SIG_DFL);
			close(listen_sock);
			debug_fprintf("(%s)(figlio %d) Connessione accettata dall'host: %s alla porta %hu\n",prog_name,getpid(),inet_ntoa(addr_sorgente.sin_addr),ntohs(addr_sorgente.sin_port));
			ricevi_dati(vettore_s,accepted_sd);
			exit(0);
		}

		/*processo padre*/
		figli[n_figli] = pid_figlio;
		n_figli++;
		close(accepted_sd);
	}

	return 0;
}


void input_file(VETTORE* numeri, char* filename){

	FILE* fp;
	float temp;
	int i;
	char buffer[LINE];

	if((fp = fopen(filename,"r"))==NULL){
		perror("Problema in apertura del file");
		exit(-1);
	}

	debug_fprintf("(%s)(parent) File aperto con successo\n",prog_name);

	for(i=0;fgets(buffer,LINE,fp)!=NULL && i<MAXLINEE;i++){
		if(sscanf(buffer,"%f",&temp)!=1) {
			debug_fprintf("(%s)(parent) Errore di formattazione del file\n",prog_name);
			exit(-1);
		}
		numeri->vettore[i] = temp;
	}

	numeri->n = i;

	if(fclose(fp)<0){
		perror("Errore in chiusura del file\n");
		exit(-1);
	}

}

void socket_setup(int* listen_sock,char* porta){

	struct sockaddr_in server_addr;

	*listen_sock = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	debug_fprintf("(%s)(parent) Socket aperta con successo.\n",prog_name);

	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(atoi(porta));

	Bind(*listen_sock, (struct sockaddr*) &server_addr,sizeof(server_addr));

	/*max 5 connessioni in entrata !*/
	Listen(*listen_sock,5);
	debug_fprintf("(%s)(parent) Server running\n",prog_name);

 }

void ricevi_dati(VETTORE vettore_s,int sd){

	Request blocco;
	int i,k=0;
	float risultato=0;
	bool_t ultimo = FALSE;
	bool_t errore = FALSE;
	FILE* sock_p = fdopen(sd,"r");

	while(ultimo==FALSE){
		if(decodifica_xdr(sock_p,&blocco)==TRUE) {
			for(i=0;i<blocco.data.data_len;i++,k++){
				debug_fprintf("(%s)(figlio %d) Ricevuto ---> %f\n",prog_name,getpid(),blocco.data.data_val[i]);
				/* Calcolo addendo
				 *
				 * c() 	--> blocco.data_val[]
				 * n 	--> vettore_s.n
				 * k 	--> k
				 * s[]	--> vettore_s.vettore[]
				 * r 	--> risultato
				 * */
				risultato += blocco.data.data_val[i] * (vettore_s.vettore[k%vettore_s.n]);
			}
			ultimo = blocco.last;
		}
		else{
			ultimo = TRUE;
			errore=TRUE;
		}
	}
	// errore = FALSE !! -> nessun errore, tutto ok ;)
	invia_risposta(sd,errore,risultato);

	/*
	 * Chiudere la connessione!!!!!
	 * */

	fclose(sock_p);
}

void invia_risposta(int sd, bool_t esito,float risultato){

	Response blocco;
	char buffer[LINE];
	int B_codificati;

	blocco.error = esito;
	blocco.result = risultato;

	B_codificati=codifica_xdr(buffer,&blocco);
	Writen(sd,buffer,B_codificati);

}

/*
 * Codifica i dati passati in "blocco" ponendo il risultato all'interno
 * di "buffer", il quale dovrà poi essere inviato sul socket.
 */
int codifica_xdr(char* buffer, Response* blocco){

	XDR xdr_stream;
	int B_codificati;

	xdrmem_create(&xdr_stream,buffer,BUF,XDR_ENCODE);

	xdr_Response(&xdr_stream,blocco);
	B_codificati = xdr_getpos(&xdr_stream);

	xdr_destroy(&xdr_stream);

	return B_codificati;
}

bool_t decodifica_xdr(FILE* sock_p,Request* blocco){

	XDR xdr_stream;

	memset(blocco,0,sizeof(Request));
	xdrstdio_create(&xdr_stream,sock_p,XDR_DECODE);

	if(xdr_Request(&xdr_stream,blocco)==FALSE) {
		debug_fprintf("(%s)(figlio %d) Errore nella decodifica dei dati in ingresso\n",prog_name,getpid());
		return FALSE;
	}
	xdr_destroy(&xdr_stream);

	return TRUE;

}

int my_signal(int signo,void (*funzione)(int signo)){

	struct sigaction azione_segnale;

	azione_segnale.sa_handler = funzione;
	sigemptyset(&(azione_segnale.sa_mask));
	azione_segnale.sa_flags= 0;
	return sigaction(signo,&azione_segnale,NULL);

}

void attendi_figli(int signo){

	pid_t pid;

	debug_fprintf("(%s)(parent) -DEBUG-->\t Ricevuto segnale SIGCHLD.\n",prog_name);

	while((pid=waitpid(-1,NULL,WNOHANG)) > 0){
		n_figli--;
		debug_fprintf("(%s)(parent) -DEBUG-->\t Il figlio %d è stato terminato.\n",prog_name,pid);
	}
	return;
}

void termina_figli(){

	int i;
	pid_t pid;

	debug_fprintf("(%s)(parent) -DEBUG-->\t Ricevuto segnale SIGINT.\n",prog_name);

	for(i=0;i<n_figli;i++){
		kill(figli[i],SIGINT);
	}

	while((pid=waitpid(-1,NULL,0)) > 0){
		n_figli--;
		debug_fprintf("(%s)(parent) -DEBUG-->\t Il figlio %d è stato terminato.\n",prog_name,pid);
	}

	exit(0);
}



