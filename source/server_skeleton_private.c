/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include <sys/time.h>
#include "table.h"
#include "server_skeleton.h"
#include "htmessages.pb-c.h"
#include "server_skeleton_private.h"
#include "stats.h"
#include <stdlib.h>
#include <stdio.h> // Add this line to include the necessary header file

extern struct statistics_t* stats;
extern pthread_rwlock_t stats_lock;

/*
* Inicializa a Estrutura de Stats
*/

struct statistics_t* stats_struct_init() {
    struct statistics_t* stats = malloc(sizeof(struct statistics_t));
    if (stats == NULL) {
        perror("[Server-Skeleton-Private] - Erro a alocar memória para statistics_t\n");
        return NULL;
    }
    stats->n_commands = 0;
    stats->elapsed_time = 0;
    stats->n_clients = 0;
    return stats;
}

/*
 * Register a command execution in statistics
 */
void register_command(struct timeval start, struct timeval end) {
    long mseconds  = (end.tv_sec  - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    pthread_rwlock_wrlock(&stats_lock);
    stats->n_commands += 1;
    stats->elapsed_time += mseconds;
    pthread_rwlock_unlock(&stats_lock);
}
