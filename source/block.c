/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include "block.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Função que cria um novo bloco de dados block_t e que inicializa 
 * os dados de acordo com os argumentos recebidos, sem necessidade de
 * reservar memória para os dados.	
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct block_t *block_create(int size, void *data) {
    if(size <= 0 || data == NULL) {
        return NULL;
    }
    struct block_t *block = (struct block_t *) malloc(sizeof(struct block_t));
    if (block == NULL) {
        return NULL;
    }
    block->datasize = size;
    block->data = data;
    return block;
}

/* Função que duplica uma estrutura block_t, reservando a memória
 * necessária para a nova estrutura.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct block_t *block_duplicate(struct block_t *b) {
    if(b == NULL || b->datasize <= 0 || b->data == NULL){
        return NULL;
    }
    struct block_t *block = (struct block_t *) malloc(sizeof(struct block_t));
    if (block == NULL) {
        return NULL;
    }
    block->datasize = b->datasize;
    block->data = (char *) malloc(b->datasize);
    if(block->data == NULL){
        free(block);
        return NULL;
    }
    memcpy(block->data, b->data, b->datasize);
    return block;
}

/* Função que substitui o conteúdo de um bloco de dados block_t.
 * Deve assegurar que liberta o espaço ocupado pelo conteúdo antigo.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int block_replace(struct block_t *b, int new_size, void *new_data) {
    if(b == NULL || b->datasize <= 0 || b->data == NULL || new_size <= 0 || new_data == NULL){
        return -1;
    }
    free(b->data);
    b->datasize = new_size;
    b->data = new_data;
    return 0;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro b,
 * libertando toda a memória por ele ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int block_destroy(struct block_t *b) {
    if(b == NULL){
        return -1;
    }
    free(b->data);
    free(b);
    return 0;
}
