/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "server_skeleton.h"
#include "server_skeleton_private.h"
#include "entry.h"
#include "htmessages.pb-c.h"
#include "table.h"
#include "entry.h"
#include "block.h"
#include "stats.h"

struct statistics_t *stats;
pthread_rwlock_t table_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t stats_lock = PTHREAD_RWLOCK_INITIALIZER;

/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna a tabela criada ou NULL em caso de erro. */
struct table_t *server_skeleton_init(int n_lists) {
    struct table_t *table = table_create(n_lists);
    if (table == NULL) {
        return NULL;
    }
    stats = stats_struct_init();
    return table;
}

/* Liberta toda a memória ocupada pela tabela e todos os recursos
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro. */
int server_skeleton_destroy(struct table_t *table) {
    return table_destroy(table);
}

/* Executa na tabela table a operação indicada pelo opcode contido em msg
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro. */
int invoke(MessageT *msg, struct table_t *table) {
    if (msg == NULL || table == NULL) {
        return -1;
    }
    struct timeval start, end;
    if (pthread_rwlock_tryrdlock(&table_lock) == 0) {
        pthread_rwlock_unlock(&table_lock);
    } else {
        pthread_rwlock_init(&table_lock, NULL);
        pthread_rwlock_init(&stats_lock, NULL);
    }
    gettimeofday(&start, NULL);
    switch (msg->opcode) {
        case MESSAGE_T__OPCODE__OP_PUT:
            int data_size = msg->entry->value.len;
            void* data = malloc(data_size);
            memcpy(data,msg->entry->value.data,data_size);
            struct block_t *put_block = block_create(data_size,data);

            pthread_rwlock_wrlock(&table_lock);
            int result = table_put(table,msg->entry->key,put_block);
            pthread_rwlock_unlock(&table_lock);

            block_destroy(put_block);

            if (result == 0) {
                msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                gettimeofday(&end, NULL);
                register_command(start, end);
                return 0;
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
        case MESSAGE_T__OPCODE__OP_GET:
            pthread_rwlock_rdlock(&table_lock);
            struct block_t *get_block = table_get(table, msg->key);
            pthread_rwlock_unlock(&table_lock);
            if (get_block == NULL) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }

            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            int size = get_block->datasize;
            void* data_get = malloc(size);
            memcpy(data_get,(void *) get_block->data, size);
            msg->value.data = data_get;
            msg->value.len = size;

            block_destroy(get_block);

            gettimeofday(&end, NULL);
            register_command(start, end);

            return 0;
        case MESSAGE_T__OPCODE__OP_DEL:
            pthread_rwlock_wrlock(&table_lock);
            int remove = table_remove(table, msg->key);
            pthread_rwlock_unlock(&table_lock);

            if (remove == 0) {
                msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

                gettimeofday(&end, NULL);
                register_command(start, end);

                return 0;
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
        case MESSAGE_T__OPCODE__OP_SIZE:
            pthread_rwlock_rdlock(&table_lock);
            msg->result = table_size(table);
            pthread_rwlock_unlock(&table_lock);

            msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

            gettimeofday(&end, NULL);
            register_command(start, end);

            return 0;
        case MESSAGE_T__OPCODE__OP_GETTABLE:
            pthread_rwlock_rdlock(&table_lock);
            char** keys = table_get_keys(table);
            if (keys == NULL) {
                pthread_rwlock_unlock(&table_lock);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            int num_entries = table_size(table);
            msg->entries = malloc((num_entries + 1) * sizeof(EntryT *));
            if (msg->entries == NULL) {
                pthread_rwlock_unlock(&table_lock);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                table_free_keys(keys);
                return -1;
            }

            for (int i = 0; i < num_entries; i++) {
                struct block_t *block = table_get(table, keys[i]);
                int size = block->datasize;
                void* data_get = malloc(size);
                memcpy(data_get,(void *) block->data, size);

                msg->entries[i] = malloc(sizeof(EntryT));
                if (msg->entries[i] == NULL) {
                    pthread_rwlock_unlock(&table_lock);
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    table_free_keys(keys);
                    return -1;
                }
                entry_t__init(msg->entries[i]);
                msg->entries[i]->key = strdup(keys[i]);
                msg->entries[i]->value.data = data_get;
                msg->entries[i]->value.len = size;
                block_destroy(block);
            }
            msg->entries[num_entries] = NULL;
            msg->n_entries = num_entries;
            table_free_keys(keys);
            pthread_rwlock_unlock(&table_lock);
            msg->opcode = MESSAGE_T__OPCODE__OP_GETTABLE + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            gettimeofday(&end, NULL);
            register_command(start, end);
            return 0;
        case MESSAGE_T__OPCODE__OP_GETKEYS:
            pthread_rwlock_rdlock(&table_lock);
            char** get_keys = table_get_keys(table);

            if (get_keys == NULL) {
                pthread_rwlock_unlock(&table_lock);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }

            int i = 0;
            while (get_keys[i] != NULL)
                i++;
            char **keys_cp = malloc((i + 1) * sizeof(char *));

            for(int j = 0; j < i; j++) {
                keys_cp[j] = strdup(get_keys[j]);
                printf("key: %s\n", keys_cp[j]);
            }
            keys_cp[i] = NULL; // Add NULL terminator
            msg->keys = keys_cp;
            msg->n_keys = i; // Set the number of keys
            table_free_keys(get_keys);
            pthread_rwlock_unlock(&table_lock);
            msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            gettimeofday(&end, NULL);
            register_command(start, end);

            return 0;
        case MESSAGE_T__OPCODE__OP_STATS:
            msg->stats = malloc(sizeof(StatisticsT));
            if (msg->stats == NULL) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }

            statistics_t__init(msg->stats);
            pthread_rwlock_rdlock(&stats_lock);
            msg->stats->total_ops = stats->n_commands;
            //printf("%d", stats.n_commands);
            msg->stats->elapsed_time = stats->elapsed_time;
            //printf("%d", stats->elapsed_time);
            msg->stats->total_clients_connected = stats->n_clients;
            //printf("%d", stats->n_clients);
            pthread_rwlock_unlock(&stats_lock);
            msg->opcode = MESSAGE_T__OPCODE__OP_STATS + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_STATS;

            gettimeofday(&end, NULL);

            return 0;
        default:
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
    }
}

/*
 * Register client disconnection in statistics
 */
void unregister_client() {
    pthread_rwlock_wrlock(&stats_lock);
    stats->n_clients--;
    pthread_rwlock_unlock(&stats_lock);
}

/*
 * Register new client in statistics
 */
void register_client() {
    pthread_rwlock_wrlock(&stats_lock);
    stats->n_clients++;
    pthread_rwlock_unlock(&stats_lock);
}

/*
 * Destroy locks
 */
void destroy_rwlock() {
    pthread_rwlock_destroy(&table_lock);
    pthread_rwlock_destroy(&stats_lock);
}


