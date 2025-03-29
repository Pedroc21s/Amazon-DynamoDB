/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include "serialization.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Serializa todas as chaves presentes no array de strings keys para o
* buffer keys_buf, que será alocado dentro da função. A serialização
* deve ser feita de acordo com o seguinte formato:
* | int | string | string | string |
* | nkeys | key1 | key2 | key3 |
* Retorna o tamanho do buffer alocado ou -1 em caso de erro.
*/
int keyArray_to_buffer(char **keys, char **keys_buf) {
    if (keys == NULL || keys_buf == NULL) {
        return -1;
    }
    
    int bufferSize = sizeof(int);

    int nkeys = 0;
    for (int i = 0; keys[i] != NULL; i++) {
        nkeys++;
        bufferSize += strlen(keys[i]) + 1;
    }
    *keys_buf = (char*)malloc(bufferSize);
    if (*keys_buf == NULL) {
        return -1; // Memory allocation failed
    }

    int nkeys_network_order = htonl(nkeys);
    memcpy(*keys_buf, &nkeys_network_order, sizeof(int));
    char *ptr = *keys_buf + sizeof(int);

    for (int i = 0; keys[i] != NULL; i++) {
        strcpy(ptr, keys[i]);
        ptr += strlen(keys[i]) + 1;
    }

    return bufferSize;
}

/* De-serializa a mensagem contida em keys_buf, colocando-a num array de
* strings cujo espaco em memória deve ser reservado. A mensagem contida
* em keys_buf deverá ter o seguinte formato:
* | int | string | string | string |
* | nkeys | key1 | key2 | key3 |
* Retorna o array de strings ou NULL em caso de erro.
*/
char** buffer_to_keyArray(char *keys_buf) {
    if (keys_buf == NULL) {
        return NULL;
    }

    int nkeys = ntohl(*((int*)keys_buf));
    char **keys = (char**)malloc((nkeys + 1) * sizeof(char*));
    if (keys == NULL) {
        return NULL;
    }

    char *ptr = keys_buf + sizeof(int);
    for (int i = 0; i < nkeys; i++) {
        keys[i] = strdup(ptr);
        if (keys[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(keys[j]);
            }
            free(keys);
            return NULL;
        }
        ptr += strlen(ptr) + 1;
    }

    keys[nkeys] = NULL;
    return keys;
}