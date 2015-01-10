/*
 ============================================================================
 Name        : 20130717-client-xdr.c
 Author      : Luca Giovagnoli
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>

#include <rpc/xdr.h>

#include "wrapsock_mod.h"
#include "QUEUElista.h"
#include "types.h"
#include "my_transfer_n.h"

#define BUF 8192
#define LINE 50
#define SEQ 3

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

void input_file(FIFO* lista_float, char* filename);
void socket_setup(int* sd,char* indirizzo,char* porta);
int invia_dati(FIFO* lista, int sd);
int ricevi_esito(int sd);
int codifica_xdr(char* buffer, Request* blocco);
void decodifica_xdr(FILE* sock_p,Response* blocco);
void stampafloat(void* numero);
int my_signal(int signo,void (*funzione)(int signo));

char *prog_name;

int main(int argc, char* argv[]) {

	FIFO* lista_float = QUEUEinit();
	int socket_connessa,esito;

	if(argc != 4){
			debug_fprintf("(%s) -ERRORE-->\t Numero errato di argomenti",prog_name);
			return -1;
		}

	prog_name=argv[0];

	input_file(lista_float,argv[3]);

	socket_setup(&socket_connessa,argv[1],argv[2]);

	my_signal(SIGPIPE,SIG_IGN);

	if(invia_dati(lista_float,socket_connessa)==1) 	esito = ricevi_esito(socket_connessa);


	QUEUEfree(lista_float,free);

	return esito;
}

void input_file(FIFO* lista_float, char* filename){

	FILE* fp;
	char buffer[LINE];
	float* temp;


	if((fp = fopen(filename,"r"))==NULL){
		debug_fprintf("(%s) ",prog_name);
		fflush(stdout);
		perror("-ERRORE-->\t Problema in apertura del file");
		exit(-1);
	}

	debug_fprintf("(%s) -SUCCESSO-->\t File aperto con successo\n",prog_name);

	while(fgets(buffer,LINE,fp)!=NULL){
		temp=(float*)malloc(sizeof(float));
		if(sscanf(buffer,"%f",temp)!=1) {
			debug_fprintf("(%s) -ERRORE-->\t Errore di formattazione del file\n",prog_name);
			exit(-1);
		}
		QUEUEinsert(lista_float,temp);
	}

	if(fclose(fp)<0){
		perror("Errore in chiusura del file\n");
		exit(-1);
	}
}

void socket_setup(int* sd,char* indirizzo,char* porta){

	struct addrinfo sugg;
	struct addrinfo* info_ind = NULL;

	memset(&sugg,0,sizeof(struct addrinfo));

	sugg.ai_family = AF_INET;
	sugg.ai_socktype = SOCK_STREAM;
	sugg.ai_protocol = IPPROTO_TCP;

	getaddrinfo(indirizzo,porta,&sugg,&info_ind);

	*sd = Socket(info_ind->ai_family, info_ind->ai_socktype, info_ind->ai_protocol);
	debug_fprintf("(%s) -SUCCESSO-->\t Socket aperta con successo.\n",prog_name);

	Connect(*sd, info_ind->ai_addr, info_ind->ai_addrlen);
	debug_fprintf("(%s) -SUCCESSO-->\t Connessione riuscita.\n",prog_name);

	freeaddrinfo(info_ind);
 }

int invia_dati(FIFO* lista, int sd){

	Request blocco;
	int i,B_codificati,len;
	int totali = QUEUEcount(lista);
	int n = totali/SEQ;
	int resto = totali%SEQ;
	char buffer[BUF];
	float* decimali = (float*)malloc(SEQ*sizeof(float));
	float* temp;

	for(;n>=0;n--){
		len=SEQ;
		if(n==0 && resto>0) len=resto;
		for(i=0;i<len;i++){
			temp = (float*) QUEUEget(lista);
			decimali[i]= *temp;
			free(temp);
		}
		blocco.data.data_len = len;
		blocco.data.data_val = decimali;

		/*Controllo se all'n-esima iterazione intera il resto è nullo. In tal caso
		 * segno il blocco come ultimo e decremento n in modo da uscire dal ciclo esterno. */
		if(n==1 && resto==0) {
			blocco.last = TRUE;
			n--; 	/* decrementando n uscirò dal ciclo esterno */
		}
		else if (n==0) blocco.last = TRUE;	/* Nel caso in cui n==0 segno senza dubbio il blocco come ultimo*/
		else blocco.last = FALSE;			/* In tutti i casi "normali" con n>1 il blocco non è l'ultimo*/

		B_codificati = codifica_xdr(buffer,&blocco);

		/****************
		 * Nel caso vi sia un errore EPIPE (broken pipe) devo smettere
		 * di inviare dati e uscire dalla funzione perché significa
		 * che la connessione è stata chiusa dal server. Dopo che il server
		 * ha chiuso la connessione se provo a scrivere (write) il server
		 * risponde con RST (la prima volta). Se scrivo nuovamente mi viene
		 * inviato il segnale SIGPIPE (e bisogna gestirlo).
		 * SIGPIPE viene ignorato da questo client. La condizione di errore
		 * viene gestita qui di seguito controllando il valore di errno.
		 * ****************/

		if(writen(sd,buffer,B_codificati)<0){
			if(errno == EPIPE) {
				debug_fprintf(" (%s) -ERROR-->\t ",prog_name);
				perror("Il server deve aver chiuso la connessione");
				free(decimali);
				return -1;
			}
		}

	}

	free(decimali);
	return 1;
}

int ricevi_esito(int sd){

	Response blocco;
	FILE* sp = fdopen(sd,"r");

	memset(&blocco,0,sizeof(blocco));
	decodifica_xdr(sp,&blocco);

	/*****************************
	 * chiudo la connessione con il server
	 *****************************/
	fclose(sp);

	if(blocco.error==FALSE){
		FILE* fp = fopen("result.txt","w");
		if(fprintf(fp,"%f",blocco.result) > 0){
			debug_fprintf("(%s) -SUCCESSO-->\t Scrittura su file avvenuta con successo\n",prog_name);
		}
		fclose(fp);
		debug_fprintf("(%s) -DEBUG   -->\t Il risultato inviato dal server vale %f\n",prog_name,blocco.result);

		/***********************
		 * ritornare con codice di uscita 0
		 * ************************/
		return 0;

	}

	else if(blocco.error == TRUE){
		debug_fprintf("(%s) -ERRORE-->\t Il server segnala un errore\n",prog_name);

		/***********************
		 * ritornare con codice di uscita 1
		 * ************************/
		return 1;
	}
	return 0;
}

int codifica_xdr(char* buffer, Request* blocco){

	XDR xdr_stream;
	int B_codificati;

	xdrmem_create(&xdr_stream,buffer,BUF,XDR_ENCODE);

	if(xdr_Request(&xdr_stream,blocco)==FALSE) debug_fprintf("(%s) -ERRORE-->\t Errore nella codifica dei dati\n",prog_name);
	B_codificati = xdr_getpos(&xdr_stream);

	xdr_destroy(&xdr_stream);

	return B_codificati;
}

void decodifica_xdr(FILE* sock_p,Response* blocco){

	XDR xdr_stream;

	xdrstdio_create(&xdr_stream,sock_p,XDR_DECODE);
	xdr_Response(&xdr_stream,blocco);

	xdr_destroy(&xdr_stream);

}

void stampafloat(void* numero){
	float* num = (float*) numero;
	printf("%f\n",*num);
}

int my_signal(int signo,void (*funzione)(int signo)){

	struct sigaction azione_segnale;

	azione_segnale.sa_handler = funzione;
	sigemptyset(&(azione_segnale.sa_mask));
	azione_segnale.sa_flags= 0;
	return sigaction(signo,&azione_segnale,NULL);

}


