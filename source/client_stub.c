/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "client_stub-private.h" // Add this line to include the definition of struct rtable_t
#include "client_network.h" // Add this line to include the declaration of network_connect
#include "htmessages.pb-c.h" // Ensure this header is correctly included
#include "client_stub.h"
#include "block.h"
#include "entry.h"
#include "server_skeleton.h"
#include "stats.h"
#include <netdb.h>

/* Função para estabelecer uma associação entre o cliente e o servidor,
* em que address_port é uma string no formato <hostname>:<port>.
* Retorna a estrutura rtable preenchida, ou NULL em caso de erro.
*/
struct rtable_t *rtable_connect(char *address_port) {

    char *hostname = strtok(address_port, ":");
    char *port_str = strtok(NULL, "");
    if (hostname == NULL || port_str == NULL) {
        fprintf(stderr, "[Client-Stub] - Invalid format of the address\n");
        return NULL;
    } 

    struct rtable_t *rtable = (struct rtable_t *)malloc(sizeof(struct rtable_t));
    if (rtable == NULL) {
        fprintf(stderr, "[Client-Stub] - A alocação de memória da rtable falhou\n");
        return NULL;
    }

    short port = atoi(port_str);
    if (port < 1024) {
        fprintf(stderr, "[Client-Stub] - Número inválido do porto\n");
        free(rtable);
        return NULL;
    }

    rtable->server_address = strdup(hostname);
    rtable->server_port = htons(port);

    if (network_connect(rtable) == -1) {
        free(rtable->server_address);
        free(rtable);
        return NULL;
    }
    return rtable;
}

/* Termina a associação entre o cliente e o servidor, fechando a
* ligação com o servidor e libertando toda a memória local. * Retorna 0 se tudo correr bem, ou -1 em caso de erro. */ 
int rtable_disconnect(struct rtable_t *rtable) {
    if (rtable == NULL) {
        fprintf(stderr, "[Client-Stub] - rtable é NULL\n");
        return -1;
    }

    int result = network_close(rtable);
    if (result == -1) {
        fprintf(stderr, "[Client-Stub] - Foi impossível fechar a conexão\n");
        return -1;
    }

    free(rtable->server_address);
    free(rtable);
    return 0;
}

/* Função para adicionar uma entrada na tabela.
* Se a key já existe, vai substituir essa entrada pelos novos dados.
* Retorna 0 (OK, em adição/substituição), ou -1 (erro).
*/
int rtable_put(struct rtable_t *rtable, struct entry_t *entry) {
    if (rtable == NULL || entry == NULL) {
        fprintf(stderr, "[Client-Stub] - Argumentos inválidos\n");
        return -1;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    EntryT entryt = ENTRY_T__INIT;
    entryt.key = entry->key;
    entryt.value.len = entry->value->datasize;
    entryt.value.data = entry->value->data;
    msg.entry = &entryt;

    MessageT *response = network_send_receive(rtable, &msg);
    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        fprintf(stderr, "[Client-Stub] - Erro a receber a resposta\n");
        message_t__free_unpacked(response, NULL);
        return -1;
    }

    message_t__free_unpacked(response, NULL);
    return 0;
}

/* Retorna a entrada da tabela com chave key, ou NULL caso não exista
* ou se ocorrer algum erro.
*/
struct block_t *rtable_get(struct rtable_t *rtable, char *key) {
    if (rtable == NULL || key == NULL) {
        fprintf(stderr, "[Client-Stub] - Argumentos inválidos\n");
        return NULL;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg.key = key;
    
    MessageT *response = network_send_receive(rtable, &msg);
    
    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    if (response->c_type == MESSAGE_T__C_TYPE__CT_VALUE) {
        struct block_t *block = block_create(response->value.len,strdup((char *) response->value.data));
        message_t__free_unpacked(response, NULL);
        return block;
    }
    
    return NULL;
}

/* Função para remover um elemento da tabela. Vai libertar
* toda a memoria alocada na respetiva operação rtable_put().
* Retorna 0 (OK), ou -1 (chave não encontrada ou erro).
*/
int rtable_del(struct rtable_t *rtable, char *key) {
    if (rtable == NULL || key == NULL) {
        fprintf(stderr, "[Client-Stub] - Argumentos inválidos\n");
        return -1;
    }

    if(rtable_get(rtable, key) == NULL) {
        return -1;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg.key = key;
    
    MessageT *response = network_send_receive(rtable, &msg);
    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        fprintf(stderr, "[Client-Stub] - Erro a receber uma resposta\n");
        message_t__free_unpacked(response, NULL);
        return -1;
    }

    message_t__free_unpacked(response, NULL);
    return 0;
}

/* Retorna o número de elementos contidos na tabela ou -1 em caso de erro.
*/
int rtable_size(struct rtable_t *rtable) {
    if (rtable == NULL) {
        fprintf(stderr, "[Client-Stub] - Argumentos inválidos\n");
        return -1;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &msg);
    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        fprintf(stderr, "[Client-Stub] - Erro a receber uma resposta\n");
        message_t__free_unpacked(response, NULL);
        return -1;
    }

    int size = response->result;
    message_t__free_unpacked(response, NULL);
    return size;
}

/* Retorna um array de char* com a cópia de todas as keys da tabela,
* colocando um último elemento do array a NULL.
* Retorna NULL em caso de erro.
*/
char **rtable_get_keys(struct rtable_t *rtable) {
    if (rtable == NULL) {
        fprintf(stderr, "[Client-Stub] - Argumentos inválidos\n");
        return NULL;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &msg);
    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        //fprintf(stderr, "[Client-Stub] - Erro a receber uma resposta\n");
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    if (response->c_type == MESSAGE_T__C_TYPE__CT_KEYS) {
        char **keys = malloc((response->n_keys + 1) * sizeof(char *));

        for (int j = 0; j < response->n_keys; j++) {
            keys[j] = strdup(response->keys[j]);
        }
        keys[response->n_keys] = NULL; // Add NULL terminator

        message_t__free_unpacked(response, NULL);
        return keys;
    }
    return NULL;
}

/* Liberta a memória alocada por rtable_get_keys().
*/
void rtable_free_keys(char **keys) {
    if (keys == NULL) {
        return;
    }

    int i = 0;
    while (keys[i] != NULL) {   
        free(keys[i]);          
        i++;
    }

    free(keys);
}

/* Retorna um array de entry_t* com todo o conteúdo da tabela, colocando
* um último elemento do array a NULL. Retorna NULL em caso de erro.
*/
struct entry_t **rtable_get_table(struct rtable_t *rtable) {
    if (rtable == NULL) {
        fprintf(stderr, "[Client-Stub] - Argumentos inválidos\n");
        return NULL;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETTABLE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &msg);
    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        fprintf(stderr, "[Client-Stub] - Erro a receber uma resposta\n");
        message_t__free_unpacked(response, NULL);
        return NULL;
    }
    
    struct entry_t **table = malloc(response->n_entries * sizeof(struct entry_t));
    if (table == NULL) {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    for (int i = 0; i < response->n_entries; i++) {
        struct block_t *b = block_create(response->entries[i]->value.len, response->entries[i]->value.data);
        table[i] = entry_create(strdup(response->entries[i]->key),block_duplicate(b));
        if (table[i] == NULL) {
            for (int j = 0; j < i; j++) {
                entry_destroy(table[j]);
            }
            free(table);
            message_t__free_unpacked(response, NULL);
            return NULL;
        }
        //block_destroy(b);
    }
    table[response->n_entries] = NULL;  // Último elemento NULL

    message_t__free_unpacked(response, NULL);
    return table;
}

/* Liberta a memória alocada por rtable_get_table().
*/
void rtable_free_entries(struct entry_t **entries) {
    if (entries == NULL) {
        return;
    }

    for (int i = 0; entries[i] != NULL; i++) {
        entry_destroy(entries[i]);
    }
    free(entries);
}

/* 
* Obtém as estatísticas do servidor.
*/
struct statistics_t rtable_stats(struct rtable_t *rtable) {
    struct statistics_t stats_t;
    
    if (rtable == NULL) {
        fprintf(stderr, "[Client-Stub] - Argumentos inválidos\n");
        stats_t.n_clients = -1;
        return stats_t;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_STATS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response = network_send_receive(rtable, &msg);
    if (response == NULL || response->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        fprintf(stderr, "[Client-Stub] - Erro a receber uma resposta\n");
        if (response != NULL) {
            message_t__free_unpacked(response, NULL);
        }
        stats_t.n_clients = -1;
        return stats_t;
    } 

    if (response->stats == NULL) {
        fprintf(stderr, "[Client-Stub] - Stats data not available in response\n");
        message_t__free_unpacked(response, NULL);
        stats_t.n_clients = -1;
        return stats_t;
    }

    stats_t.n_clients = response->stats->total_clients_connected;
    stats_t.elapsed_time = response->stats->elapsed_time;
    stats_t.n_commands = response->stats->total_ops;
    
    message_t__free_unpacked(response, NULL);
    return stats_t;
}
