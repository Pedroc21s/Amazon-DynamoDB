/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include "block.h"
#include "entry.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Função que cria uma entry, reservando a memória necessária e
 * inicializando-a com a string e o bloco de dados de entrada.
 * Retorna a nova entry ou NULL em caso de erro.
 */
struct entry_t *entry_create(char *key, struct block_t *value) {
    if(key == NULL || value == NULL) {
        return NULL;
    }
    struct entry_t *entry = (struct entry_t *) malloc(sizeof(struct entry_t));
    if(entry == NULL) {
        return NULL;
    }
    entry->key = key;
    entry->value = value;
    return entry;
}

/* Função que compara duas entries e retorna a ordem das mesmas, sendo esta
 * ordem definida pela ordem das suas chaves.
 * Retorna 0 se as chaves forem iguais, -1 se e1 < e2,
 * 1 se e1 > e2 ou -2 em caso de erro.
 */
int entry_compare(struct entry_t *e1, struct entry_t *e2) {
    if(e1 == NULL || e2 == NULL || e1->key == NULL || e1->value == NULL || e2->key == NULL || e2->value == NULL) {
        return -2;
    }
    int cmp = strcmp(e1->key, e2->key);
    if(cmp > 0) {
        return 1;
    } else if (cmp < 0) {
        return -1;
    }
    return cmp;
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 * Retorna a nova entry ou NULL em caso de erro.
 */
struct entry_t *entry_duplicate(struct entry_t *e) {
    if(e == NULL || e->key == NULL || e->value == NULL) {
        return NULL;
    }
    char *key = (char*) malloc(sizeof(char));
    if (key == NULL) { 
        return NULL; 
    }
    strcpy(key, e->key);
	struct block_t *value = block_duplicate(e->value);
    if (value == NULL) {
        free(key);
        return NULL;
    }
    struct entry_t *copy = entry_create(key,value);
    if(copy == NULL) {
        free(key);
        free(value);
        return NULL;
    }
    return copy;
}

/* Função que substitui o conteúdo de uma entry, usando a nova chave e
 * o novo valor passados como argumentos, e eliminando a memória ocupada
 * pelos conteúdos antigos da mesma.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */ 
int entry_replace(struct entry_t *e, char *new_key, struct block_t *new_value) {
    if(e == NULL || e->key == NULL || e->value == NULL || new_key == NULL || new_value == NULL) {
        return -1;
    }
    if (block_destroy(e->value) == -1) {
        return -1;   
    }
    free(e->key);
    e->key = new_key;
    e->value = new_value;
    return 0;
}

/* Função que elimina uma entry, libertando a memória por ela ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int entry_destroy(struct entry_t *e) {
    if(e == NULL || e->key == NULL || e->value == NULL) {
        return -1;
    }
    if (block_destroy(e->value) == -1) {
        return -1;
    }
    free(e->key);
    free(e);
    return 0;
}