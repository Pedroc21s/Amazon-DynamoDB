#ifndef _SERVER_HASHTABLE_H
#define _SERVER_HASHTABLE_H

#include "client_stub.h"
#include <zookeeper/zookeeper.h>

typedef struct String_vector zoo_string; 

struct socket_args {
	int socket;
	struct table_t *table;
};

int init_server(short server_port, int n_lists);

int connect_to_zookeeper(char* zookeeper_port);

int create_server_node(char* server_port);

int synchronize_server_zookeeper();

int update_next_server(char* n_server);

int update_local_table_by_server(char* p_server);

int main(int argc, char *argv[]);

void stop_execution(int signal);

void close_zookeeper_connection();

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

int get_id(char* server_path);

char* get_full_server_address(char* server_port);

int get_next_server_index(zoo_string* children_list);

int get_previous_server_index(zoo_string* children_list);

#endif