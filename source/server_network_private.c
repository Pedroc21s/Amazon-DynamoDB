/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <pthread.h>
#include "server_network.h"
#include "server_skeleton.h"
#include "server_skeleton_private.h"
#include "server_hashtable.h"
#include "client_stub.h"

extern int global_terminate;
extern struct table_t* table;
extern struct rtable_t* next_server;
extern char* next_server_node_path;

#define MAX_SOCKETS 20

/*
* Função que executa a thread de um socket
*/
void *execute_socket(void *val) {
    struct socket_args args = *(struct socket_args *) val;
    printf("[Server-Network-Private] - Cliente conectado\n");
    MessageT *message_received;
    while ((message_received = network_receive(args.socket)) != NULL) {
        if (message_received->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
            perror("[Server-Network-Private] - Erro a receber MessageT\n");
            break;
        }
        printf("[Server-Network-Private] - Recebeu mensagem com opcode %d\n", message_received->opcode);
        if (invoke(message_received,table) == -1) {
            perror("[Server-Network-Private] - Erro a executar na table a operação de MessageT\n");
        } else if (next_server != NULL) {
            if (message_received->opcode == MESSAGE_T__OPCODE__OP_PUT + 1) {
                int value_size = message_received->entry->value.len;
                void* copied_value = malloc(value_size);
                memcpy(copied_value, message_received->entry->value.data,value_size);
                struct entry_t *entry = entry_create(strdup(message_received->entry->key),block_create(value_size,copied_value));
                rtable_put(next_server, entry);             
            } else if (message_received->opcode == MESSAGE_T__OPCODE__OP_DEL + 1) {
                printf("[Server-Network-Private] - A propagar operação de delete da key %s", message_received->key);
                rtable_del(next_server, message_received->key);
            }
        } 
        printf("[Server-Network-Private] - Recebeu mensagem com opcode %d\n", message_received->opcode);

        if (network_send(args.socket, message_received) == -1) {
            perror("[Server-Network-Private] - Erro a enviar MessageT\n");
            break;
        }
        message_t__free_unpacked(message_received, NULL);
    }
    unregister_client();
    printf("[Server-Network-Private] - Socket do Cliente foi fechada com sucesso\n");
    close(args.socket);
    free(val);
    pthread_exit(NULL);
    return NULL;
}

int create_new_thread(int connected_socket, struct table_t *table, pthread_t *socket_threads) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (socket_threads[i] == 0) {
            pthread_t thread;
            struct socket_args* arg = malloc(sizeof(struct socket_args));
            arg->socket = connected_socket;
            arg->table = table;
            register_client();
            pthread_create(&thread, NULL, execute_socket, arg);
            socket_threads[i] = thread;
            return i;
        }
    }
    return -1;
}

void wait_for_threads(pthread_t *socket_threads[]) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (socket_threads[i] != 0) {
            pthread_join(*socket_threads[i], NULL);
        }
    }
}

char* get_local_ip_address(int sockfd) {
    char* ip_address;
    char buffer[1024];
    FILE* fp = popen("hostname -I", "r");
    if (fp == NULL) {
        return NULL;
    }
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char *token = strtok(buffer, " \t\n");
        if (token != NULL) {
            ip_address = strdup(token);
        }
    }
    pclose(fp);
    return ip_address;
}

