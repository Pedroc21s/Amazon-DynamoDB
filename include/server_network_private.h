#ifndef _SERVER_NETWORK_PRIVATE_H
#define _SERVER_NETWORK_PRIVATE_H

#include "table.h"
#include "server_network.h"
#include "htmessages.pb-c.h"
#include <pthread.h>

/*
 * Cria uma nova thread para um socket
 */
int create_new_thread(int connected_socket, struct table_t *table, pthread_t *socket_threads);

/*
* Função que executa a thread de um socket
*/
void execute_socket(void* val);

/*
 * Espera que todas as threads terminem
 */
void wait_for_threads(pthread_t *socket_threads);

/*
 * Get the IP address of the server
 */
char* get_local_ip_address(int sockfd);


#endif
