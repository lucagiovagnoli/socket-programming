/*
 *  QUEUE.h
 *  Es1(ADT-categoria1-FIFO)
 *
 *  Created by Luca Giovagnoli on 06/12/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

typedef struct fifo FIFO;

FIFO* QUEUEinit();
int QUEUEcount(FIFO* coda);
void* QUEUEget(FIFO* coda);
void QUEUEinsert(FIFO* coda,void* info);
void QUEUEfree(FIFO* coda,void (*free_info)(void*));
void QUEUEstampa(FIFO* coda, void (*stampa_info)(void*));
