/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H /* Módulo TABLE_PRIVATE */

#include "list.h"

/* Esta estrutura define o par {chave, valor} para a tabela
 */
struct table_t {
	int numLists;
	struct list_t **content;
};

/* Esta função define como se vai fazer o hashing dentro do table.c
 */
int get_hashcode(char* key, int size);

#endif