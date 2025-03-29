#ifndef _SERVER_SKELETON_PRIVATE_H
#define _SERVER_SKELETON_PRIVATE_H

#include <pthread.h>
#include <sys/time.h>
#include "table.h"
#include "server_skeleton.h"
#include "htmessages.pb-c.h"
#include "stats.h"

/*
 * Register new command data in statistics
 */
void register_command(struct timeval start, struct timeval end);

struct statistics_t* stats_struct_init();

#endif
