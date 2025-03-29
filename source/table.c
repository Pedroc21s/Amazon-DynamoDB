/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include "block.h"
#include "table.h"
#include "table-private.c"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Função para criar e inicializar uma nova tabela hash, com n
 * linhas (n = módulo da função hash).
 * Retorna a tabela ou NULL em caso de erro.
 */
struct table_t *table_create(int n) {
    struct table_t *table = (struct table_t *)malloc(sizeof(struct table_t));
    if (table == NULL) {
        perror("[Table] - Error allocating space for the table structure");
        return NULL;
    }
    
    table->numLists = n;
    table->content = (struct list_t **)calloc(n, sizeof(struct list_t *));

    if (table->content == NULL) {
        perror("[Table] - Error while allocating the space for the table");
        free(table);
        return NULL;
    }

    for (int i = 0; i < n; i++) {
        table->content[i] = list_create();
        if (table->content[i] == NULL) {
            perror("[Table] - Error while allocating space for a list");
            for (int j = 0; j < i; j++) {
                free(table->content[j]);
            }
            free(table->content);
            free(table);
            return NULL;
        }
    }
    return table;
}

/* Função para adicionar um par chave-valor à tabela. Os dados de entrada
 * desta função deverão ser copiados, ou seja, a função vai criar uma nova
 * entry com *CÓPIAS* da key (string) e dos dados. Se a key já existir na
 * tabela, a função tem de substituir a entry existente na tabela pela
 * nova, fazendo a necessária gestão da memória.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int table_put(struct table_t *t, char *key, struct block_t *value) {
    if (key == NULL || value == NULL) { return -1; }

    struct list_t *list = t->content[get_hashcode(key, t->numLists)];
    char* keyCopy = strdup(key);
    struct block_t *valueCopy = block_duplicate(value);
    struct entry_t *entry = entry_create(keyCopy, valueCopy);
    int result = list_add(list, entry);

    // O result é 1 se a entry tiver sido substituido, 0 se nao, -1 em caso de erro
    if (result == -1) {
        free(keyCopy);
        block_destroy(valueCopy);
        return -1;
    }
    else {
        return 0;
    }

}

/* Função que procura na tabela uma entry com a chave key. 
 * Retorna uma *CÓPIA* dos dados (estrutura block_t) nessa entry ou 
 * NULL se não encontrar a entry ou em caso de erro.
 */
struct block_t *table_get(struct table_t *t, char *key) {
    struct list_t *list = t->content[get_hashcode(key, t->numLists)];
    struct entry_t *entry = list_get(list, key);
    if (entry == NULL) { return NULL; }
    return block_duplicate(entry->value);
}

/* Função que conta o número de entries na tabela passada como argumento.
 * Retorna o tamanho da tabela ou -1 em caso de erro.
 */
int table_size(struct table_t *t) {
    int size = 0;
    for (int i = 0; i < t->numLists; i++) {
        size += t->content[i]->size;
    }
    return size;
}

/* Função auxiliar que constrói um array de char* com a cópia de todas as keys na 
 * tabela, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 * Retorna o array de strings ou NULL em caso de erro.
 */
char **table_get_keys(struct table_t *t) {
   if (t == NULL || t->numLists == 0) {
        return NULL;  
    } 

    char **total = (char **) malloc(sizeof(char *) * (table_size(t) + 1));
    if (total == NULL) { return NULL; } // Erro a alocar memória
    int indexTotal = 0;
    
    for (int i = 0; i < t->numLists; i++) {
        char** currentKeys = list_get_keys(t->content[i]);
        if (currentKeys == NULL) { continue; } // Se a lista não tiver keys ignora-se
        for (int j = 0; j < t->content[i]->size; j++) {
            total[indexTotal] = strdup(currentKeys[j]);
            // Se houver um erro no strdup aborta-se
            if (total[indexTotal] == NULL) {
                for (int k = 0; k < indexTotal; k++) {
                free(total[k]);
            }
            free(total);
            return NULL;
            }
            indexTotal++;
        }
        // Se houver um erro a libertar a memória aborta-se
        if (list_free_keys(currentKeys) == -1) { 
            for (int k = 0; k < indexTotal; k++) {
                free(total[k]);
            }
            free(total);
            return NULL;
        }
    }
    total[indexTotal] = NULL;
    return total;
}

/* Função auxiliar que liberta a memória ocupada pelo array de keys obtido pela 
 * função table_get_keys.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_free_keys(char **keys) {
    if (keys == NULL) {
        return -1;
    }
    for (int i = 0; keys[i] != NULL; i++) {
        free(keys[i]);
    }
    free(keys);
    return 0;
}

/* Função que remove da lista a entry com a chave key, libertando a
 * memória ocupada pela entry.
 * Retorna 0 se encontrou e removeu a entry, 1 se não encontrou a entry,
 * ou -1 em caso de erro.
 */
int table_remove(struct table_t *t, char *key) {
    return list_remove(t->content[get_hashcode(key, t->numLists)], key);
}

/* Função que elimina uma tabela, libertando *toda* a memória utilizada
 * pela tabela.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_destroy(struct table_t *t) {
    for (int i = 0; i < t->numLists; i++) {
        list_destroy(t->content[i]);
    }
    free(t->content);
    free(t);
    return 0;
}