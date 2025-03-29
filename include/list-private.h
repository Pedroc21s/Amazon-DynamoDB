/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H /* Módulo LIST_PRIVATE */

#include "entry.h"

struct node_t {
    struct entry_t *e;
    struct node_t *next;
};

struct list_t {
    struct node_t *head;
    int size;
};

#endif
