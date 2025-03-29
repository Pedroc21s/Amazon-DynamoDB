/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include "block.h"
#include "entry.h"
#include "list-private.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Função que cria e inicializa uma nova lista (estrutura list_t a
 * ser definida pelo grupo no ficheiro list-private.h).
 * Retorna a lista ou NULL em caso de erro.
 */
struct list_t *list_create() {
    struct list_t *l = (struct list_t *) malloc(sizeof(struct list_t));
    if(l == NULL) {
        return NULL;
    }
    l->size = 0;
    l->head = NULL;
    return l;
}

/* Função que adiciona à lista a entry passada como argumento.
 * A entry é inserida de forma ordenada, tendo por base a comparação
 * de entries feita pela função entry_compare do módulo entry e
 * considerando que a entry menor deve ficar na cabeça da lista.
 * Se já existir uma entry igual (com a mesma chave), a entry
 * já existente na lista será substituída pela nova entry,
 * sendo libertada a memória ocupada pela entry antiga.
 * Retorna 0 se a entry ainda não existia, 1 se já existia e foi
 * substituída, ou -1 em caso de erro.
 */
int list_add(struct list_t *l, struct entry_t *entry) {
    if(l == NULL || entry == NULL) {
        return -1;
    }

    // Criar o novo node que vai ser adicionado
    struct node_t *new = (struct node_t*) malloc(sizeof(struct node_t));
    if (new == NULL) {
        return -1;
    }
    new->e = entry;
    new->next = NULL;

    // Se a lista estiver vazia
    if(l->head == NULL) {
        l->head = new;
        l->size++;
        return 0;
    }

    struct node_t *current = l->head, *previous;

    while(current != NULL && entry_compare(current->e, entry) != 1) {
        // Se a key do node já existir dá-se replace
        if(entry_compare(current->e, entry) == 0) {
            entry_destroy(current->e);
            current->e = entry; 
            free(new);
            return 1;
        }
        previous = current;
        current = current->next;
    }
    if(current == NULL) { // Adicionar a key ao fim da lista
        previous->next = new;
        l->size++;
        return 0;
    }
    if (current == l->head) { // Adicionar a key no inicio da lista
        l->head = new;
        l->head->next = current;
        l->size++;
        return 0;
    }
    // Adicionar a key no meio da lista
    previous->next = new;
    new->next = current;
    l->size++;
    return 0;
}

/* Função que conta o número de entries na lista passada como argumento.
 * Retorna o tamanho da lista ou -1 em caso de erro.
 */
int list_size(struct list_t *l) {
    if(l == NULL) {
        return -1;
    }
    return l->size;
}

/* Função que obtém da lista a entry com a chave key.
 * Retorna a referência da entry na lista ou NULL se não encontrar a
 * entry ou em caso de erro.
*/
struct entry_t *list_get(struct list_t *l, char *key) {
    if(l == NULL || key == NULL) {
        return NULL;
    }
    struct node_t *current = l->head;
    while(current != NULL) {
        // Caso em que encontrou a key
        if(strcmp(current->e->key, key) == 0) {
            return current->e;
        }
        current = current->next;
    }
    return NULL; // Não encontrou a key
}

/* Função auxiliar que constrói um array de char* com a cópia de todas as keys na 
 * lista, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 * Retorna o array de strings ou NULL em caso de erro.
 */
char **list_get_keys(struct list_t *l) {
    if(l == NULL || l->size <= 0) {
        return NULL;
    }
    char** keys = (char **) malloc(sizeof(char *) * (l->size + 1));
    if (keys == NULL) {
        return NULL;  
    }
    struct node_t *current = l->head; 
    int i = 0;
    while(current != NULL) { // Itera cada key e concateneia a sua representação textual ao char** keys
        keys[i] = strdup(current->e->key);
        current = current->next;
        i++;
    }
    keys[i] = NULL;
    return keys;
}

/* Função auxiliar que liberta a memória ocupada pelo array de keys obtido pela 
 * função list_get_keys.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_free_keys(char **keys) {
    if(keys == NULL) {
        return -1;
    }
    int i = 0;
    while (keys[i] != NULL) {
        free(keys[i]);      
        i++;
    }
    free(keys);
    return 0;
}

/* Função que elimina da lista a entry com a chave key, libertando a
 * memória ocupada pela entry.
 * Retorna 0 se encontrou e removeu a entry, 1 se não encontrou a entry,
 * ou -1 em caso de erro.
 */
int list_remove(struct list_t *l, char *key) {
    if(l == NULL || key == NULL) {
        return -1;
    }

    struct node_t *current = l->head, *previous;
    // Encontrar o node com a key respetiva a remover
    while(current != NULL && strcmp(current->e->key, key) != 0) {
        previous = current;
        current = current->next;
    }
    // Se o node com a respetiva key não existe
    if(current == NULL) {
        return 1;
    }
  
    if(current == l->head) { // Se o node estiver no inicio da lista
        l->head = current->next;
    }
    else {
        previous->next = current->next;
    }

    l->size--;
    entry_destroy(current->e);
    free(current);
    return 0;
}

/* Função que elimina uma lista, libertando *toda* a memória utilizada
 * pela lista (incluindo todas as suas entradas).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_destroy(struct list_t *l) {
    if(l == NULL) {
        return -1;
    }
    struct node_t *curr = l->head;
    while (curr != NULL) {
        struct node_t *next = curr->next;
        if (curr->e != NULL) {
            entry_destroy(curr->e);
        }
        free(curr);
        curr = next;
    }
    free(l);
    return 0;
}
