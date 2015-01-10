/*
 *  QUEUE.c
 *  Es1(ADT-categoria1-FIFO)
 *
 *  Created by Luca Giovagnoli on 06/12/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "QUEUElista.h"

typedef struct elemento{
	void* informazioni;
	struct elemento* next;
}ELEMENTO;

struct fifo{
	struct elemento* head;
	struct elemento* tail;
	int n;
};

/*  FIFO typedefinita nell'header */
FIFO* QUEUEinit(){
	FIFO* coda = (FIFO*) malloc(sizeof(FIFO));
	coda->head = NULL;
	coda->tail = NULL;
	coda->n=0;
	return coda;
}

int QUEUEcount(FIFO* coda){
	return coda->n;
}

void* QUEUEget(FIFO* coda){

	if(QUEUEcount(coda)==0){return NULL;}

	void* info = coda->head->informazioni;
	ELEMENTO* libero = coda->head;
	coda->head = coda->head->next;
	free(libero);
	(coda->n)--;
	return info;
}

void QUEUEinsert(FIFO* coda,void* info){
	
	ELEMENTO* nuovo = (ELEMENTO*)malloc(sizeof(ELEMENTO));
	nuovo->informazioni = info;
	nuovo->next = NULL;
	
	if (QUEUEcount(coda)==0) {	//se la coda è vuota
		coda->head = nuovo;
		coda->tail = coda->head;
		(coda->n)++;
		return;
	}
	
	coda->tail->next = nuovo;
	coda->tail = coda->tail->next;
	(coda->n)++;
}

void QUEUEfree(FIFO* coda,void (*free_info)(void*)){

	ELEMENTO* temp = coda->head;
	ELEMENTO* old = coda->head;

	while (temp!= NULL) {
		free_info(temp->informazioni);
		temp = temp->next;
		free(old);
		old = temp;
	}
	free(coda);
}

void QUEUEstampa(FIFO* coda, void (*stampa_info)(void*)){

	ELEMENTO* temp = coda->head;
	
	if(QUEUEcount(coda)==0){
		printf("Niente da stampare!\n");
		return;
	}

	while (temp!= NULL) {
		stampa_info(temp->informazioni);
		temp = temp->next;
	}
}



