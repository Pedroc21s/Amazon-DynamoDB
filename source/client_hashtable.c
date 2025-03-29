/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <zookeeper/zookeeper.h>
#include <unistd.h>
#include "client_stub.h"
#include "client_stub-private.h"
#include "block.h"
#include "entry.h"
#include "stats.h"

#define MAX_INPUT_SIZE 256

typedef struct String_vector zoo_string; 

// Zookeeper Constants
static char *watcher_ctx = "ZooKeeper Data Watcher";
static char *ROOT_PATH = "/chain";
static char *NODE_PATH = "/chain/node";
// Zookeeper Variables
zhandle_t *zh;
int is_connected_to_zookeeper = 0;
struct rtable_t* head_server; 
struct rtable_t* tail_server;
char* head_server_path = "";
char* tail_server_path = "";

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected_to_zookeeper = 1; 
		} else {
			is_connected_to_zookeeper = 0; 
		}
	}
}

/*
* Retorna o ID do Servidor dado
*/
int get_id(char* server) {
    char* id = strtok(server,NODE_PATH);
    return atoi(id);
}


/*
*  Retorna o Index do Servidor na Head
* Requer que haja pelo menos um Servidor 
*/
int get_current_head_server(zoo_string* children_list) {
    if (children_list->count == 0) { return -1; }
    if (children_list->count == 1) { return 0; }
    int head_index = 0;
    int head_id = get_id(children_list->data[0]);
    for (int i = 1; i < children_list->count; i++) {
        int current_id = get_id(children_list->data[i]);
        if (current_id < head_id) {
            head_id = current_id;
            head_index = i;
        }
    }
    return head_index;
}

/*
*  Retorna o Index do Servidor na Tail
* Requer que haja pelo menos um Servidor 
*/
int get_current_tail_server(zoo_string* children_list) {
    if (children_list->count == 0) { return -1; }
    if (children_list->count == 1) { return 0; }
    int tail_index = 0;
    int tail_id = get_id(children_list->data[0]);
    for (int i = 1; i < children_list->count; i++) {
        int current_id = get_id(children_list->data[i]);
        if (current_id > tail_id) {
            tail_id = current_id;
            tail_index = i;
        }
    }
    return tail_index;
}

int connect_to_head_server(zoo_string* children_list, int head_index) {
    // Obter o Full Path do novo Head Server
    char new_head_server_path[24] = "";
    strcat(new_head_server_path,ROOT_PATH); 
    strcat(new_head_server_path,"/"); 
    strcat(new_head_server_path,children_list->data[head_index]); 
    // Obter o Endereço IP do novo Head Server
    char head_server_ip[128];
    int head_server_ip_size = sizeof(head_server_ip);
    if(ZOK != zoo_get(zh,new_head_server_path,0,head_server_ip,&head_server_ip_size,NULL)){
        printf("[Client-Hashtable] - Não foi possivel obter o Endereço IP do Head Server\n");
        return -1;
    }  
    // Atualizar as variaveis, definindo então o novo Head Server
    head_server_path = children_list->data[head_index];
    head_server = rtable_connect(head_server_ip);
    if (head_server == NULL){
        printf("[Client-Hashtable] - Não foi possível conectar ao Head Server\n");
        head_server_path = "";
        return -1;
    }
    printf("[Client-Hashtable] - Definido com sucesso o Head Server Atualizado\n");
    printf("[Client-Hashtable] - Head Server com ID %d\n", get_id(children_list->data[head_index]));
    return 0;
}

int connect_to_tail_server(zoo_string* children_list, int tail_index) {
    // Obter o Full Path do novo Tail Server
    char new_tail_server_path[24] = "";
    strcat(new_tail_server_path,ROOT_PATH); 
    strcat(new_tail_server_path,"/"); 
    strcat(new_tail_server_path,children_list->data[tail_index]); 
    // Obter o Endereço IP do novo Tail Server
    char tail_server_ip[128];
    int tail_server_ip_size = sizeof(tail_server_ip);
    if(ZOK != zoo_get(zh,new_tail_server_path,0,tail_server_ip,&tail_server_ip_size,NULL)){
        printf("[Client-Hashtable] - Não foi possivel obter o Endereço IP do novo Tail Server\n");
        return -1;
    }  
    // Atualizar as variaveis, definindo então o novo Tail Server
    tail_server_path = children_list->data[tail_index];
    tail_server = rtable_connect(tail_server_ip);
    if (tail_server == NULL){
        printf("[Client-Hashtable] - Não foi possível conectar ao novo Tail Server\n");
        tail_server_path = "";
        return -1;
    }
    printf("[Client-Hashtable] - Definido com sucesso o novo Tail Server\n");
    printf("[Client-Hashtable] - Novo Tail Server com ID %d\n", get_id(children_list->data[tail_index]));
    return 0;
}

/**
* Data Watcher function for /MyData node
*/
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    while(1){
        if (state == ZOO_CONNECTED_STATE && type == ZOO_CHILD_EVENT) {
            zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
            // Obter a Lista Atualizada de Servidores Ativos, retornar se não houverem Servidores Ativos
            if (ZOK != zoo_wget_children(wzh, ROOT_PATH, child_watcher, watcher_ctx, children_list) || children_list->count == 0) {
                printf("[Client-Hashtable] - Não existem Servidores Ativos\n");
                sleep(30);
                break;
            }
            // Obter os Index's dos atuais Head e Tail Server
            int new_head_index = get_current_head_server(children_list);
            int new_tail_index = get_current_tail_server(children_list);
            // Se o Head Server e o Tail Server se mantiverem iguais retornar
            if (strcmp(head_server_path, children_list->data[new_head_index]) == 0 && strcmp(tail_server_path, children_list->data[new_tail_index]) == 0) {
                printf("[Client-Hashtable] - Os Servidores Head e Tail mantêm-se iguais\n");
                return;
            }
            // Se o Novo Head Server for diferente do Antigo então atualizar respetivamente
            if(strcmp(head_server_path, children_list->data[new_head_index]) != 0){
                printf("[Client-Hashtable] - O Head Server foi atualizado\n");
                if (strcmp(head_server_path,"") != 0) {
                    head_server_path = "";
                    rtable_disconnect(head_server);
                }
                // Se o Head é atualizado é porque foi removido um Servidor, logo pode não ser preciso dar disconnect
                connect_to_head_server(children_list, new_head_index);
            }

            // Se o novo Tail Server for diferente do Antigo então atualizar respetivamente
            if(strcmp(tail_server_path, children_list->data[new_tail_index]) != 0){
                // Se o Tail é atualizado é porque foi adicionado um Servidor, logo  é preciso dar disconnect
                // Se anteriormente havia algum Servidor Tail conectado, desconectar
                if (strcmp(tail_server_path,"") != 0) {
                    tail_server_path = "";
                    rtable_disconnect(tail_server);
                }
                // Estabelecer a conexão ao novo Servidor Tail
                connect_to_tail_server(children_list, new_tail_index);
            }
            // Sair do Loop
            break;
        }
    }
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    if (argc != 2) {
        printf("[Client-Hashtable] - Deve fazer ./client_hashtable <local_ip>:<zookeeper_port> \n");
        return -1;
    }
    char* host_port = argv[1];
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
	zh = zookeeper_init(host_port, connection_watcher, 2000, 0, 0, 0);
	if (zh == NULL) {
		fprintf(stderr,"[Client-Hashtable] - Erro a conectar ao Servidor do ZooKeeper\n");
		return -1;
	}
    // Conectar ao Servidor ZooKeeper e procurar Servidores Ativos para o Cliente se conectar
    zoo_string* children_list;
    while(1) {
        if (is_connected_to_zookeeper) {
            // Se existe o /chain
            if (ZNONODE == zoo_exists(zh, ROOT_PATH, 0, NULL)) {
                printf("[Client-Hashtable] - Não existe um /chain\n");
                sleep(30);
                continue;
            }
            // Obter a lista de Servidores Ativos
            children_list =	(zoo_string *) malloc(sizeof(zoo_string));
            if (ZOK != zoo_wget_children(zh, ROOT_PATH, &child_watcher, watcher_ctx, children_list)) {
                printf("[Client-Hashtable] - Erro a obter a lista de Servidores Ativos \n");
                sleep(30);
                free(children_list);
                continue;
            }
            // Verificar se há Servidores Ativos
            if(children_list->count == 0){
                printf("[Client-Hashtable] - Não existem Servidores Ativos\n");
                sleep(30);
                free(children_list);
                continue;
            }
            // Há Servidores Ativos para o Cliente se conectar
            break;
		}
    }
    // Se só houver um Servidor Ativo, defini-lo como Head e como Server
    if (children_list->count == 1) {
        connect_to_head_server(children_list,0);
        tail_server_path = strdup(head_server_path);
        tail_server = head_server;
    }
    else {
        // Obter os Index's dos atuais Head e Tail Server
        int head_index = get_current_head_server(children_list);
        int tail_index = get_current_tail_server(children_list);
        // Estabelecer a conexão ao Servidor Head
        if (connect_to_head_server(children_list,head_index) == -1) {
            printf("[Client-Hashtable] - Erro a conectar ao Servidor Head\n");
            return -1;
        }
        // Estabelecer a conexão ao Servidor Tail
        if (connect_to_tail_server(children_list,tail_index) == -1) {
            printf("[Client-Hashtable] - Erro a conectar ao Servidor Tail\n");
            return -1;
        }
    }

    char *command, *key;
    void *value;
    char input [MAX_INPUT_SIZE];

    printf("[Client-Hashtable] - The client was connected to the server\n");
    int notQuit = 1;
    while (notQuit) {
        printf("Command: ");
        if(fgets(input, sizeof(input), stdin) == NULL) {
            perror("[Client-Hashtable] - Error reading the command\n");
            continue;
        }
        
        input[strcspn(input, "\n")] = 0;

        if(strcmp(input, "size") == 0) {
            int size = rtable_size(tail_server);
            if(size == -1) {
                printf("[Client-Hashtable] - Error obtaining the size of the table\n");
            } 
            else {
                printf("Table size: %d\n", size);
            }
        } 
        else if(strcmp(input, "getkeys") == 0) {
            char **keys = rtable_get_keys(tail_server);
            if(keys != NULL) {
                int i = 0;
                while(keys[i] != NULL) {
                    printf("%s\n", keys[i]);
                    i++;
                }
                rtable_free_keys(keys);
            } 
            else {
                printf("[Client-Hashtable] - error obtaining keys\n");
            }
        } 
        else if(strcmp(input, "gettable") == 0) {
            struct entry_t **table = rtable_get_table(tail_server);
            if (table != NULL) {
                int i = 0;
                while(table[i] != NULL) {
                    printf("%s :: %s\n", table[i]->key, (char *) table[i]->value->data);
                    entry_destroy(table[i]);
                    i++;
                }
                free(table);
            } 
            else {
                printf("[Client-Hashtable] - Error obtaining the table\n");
            }
        } 
        else if (strcmp(input, "stats") == 0) {
            struct statistics_t stats_t = rtable_stats(tail_server);
            if(stats_t.n_clients != -1) {
                printf("Number of Operations made in the Server: %d\n",stats_t.n_commands);
                printf("Elapsed Time: %d microseconds\n", (int) stats_t.elapsed_time);
                printf("Number of Clients connected to the Server: %d\n",stats_t.n_clients);
            } 
            else {
                printf("[Client-Hashtable] - Error obtaining stats\n");
            }
        }
        else if(strcmp(input, "quit") == 0) {
            printf("Bye bye!\n");
            notQuit = 0;
        } 
        else {
            command = strtok(input, " ");

            if(command == NULL) {
                continue;
            } 
            else if(strcmp(command, "put") == 0 || strcmp(command, "p") == 0) {
                key = strtok(NULL, " ");
                value = strtok(NULL,"");

                if(key == NULL || value == NULL) {
                    printf("Invalid arguments. Usage: put <key> <value>\n");
                } 
                else {
                    char* copied_key = strdup(key);
                    int value_size = (strlen(value)+1)*sizeof(char);
                    void* copied_value = malloc(value_size);
                    memcpy(copied_value,value,value_size);
                    struct entry_t *e = entry_create(copied_key,block_create(value_size,copied_value));
                    if(rtable_put(head_server, e) == -1) {
                        printf("[Client-Hashtable] - Error storaging the item\n");
                    } 
                    entry_destroy(e);
                }
            } 
            else if(strcmp(command, "get") == 0) {
                key = strtok(NULL, " ");
                
                if(key == NULL) {
                    printf("Invalid arguments. Usage: get <key>\n");
                } 
                else {
                    struct block_t *b = rtable_get(tail_server, key);
                    if(b != NULL) {
                        printf("%s\n", (char *) b->data);
                        block_destroy(b);
                    } 
                    else {
                        printf("Error in rtable_get or key not found!\n");
                    }
                }
            } 
            else if(strcmp(command, "del") == 0) {
                key = strtok(NULL, " ");
                
                if(key == NULL) {
                    printf("Invalid arguments. Usage: del <key>\n");
                } 
                else {
                    int result = rtable_del(head_server, key);
                    if(result == 0) {
                        printf("Entry removed\n");
                    } 
                    else {
                        printf("Error in rtable_del or key not found!\n");
                    }
                }
            } 
            else {
                printf("[Client-Hashtable] - Command not recognized\n");
                printf("Usage: put <key> <value> | get <key> | del <key> | size | getkeys | gettable | stats | quit \n");
            }
        }
    }

    // Se o Head e o Tail forem o mesmo Servidor então disconectar de um deles apenas
    if (strcmp(head_server_path,tail_server_path) == 0) {
        rtable_disconnect(head_server);
        return 0;
    }


    if (rtable_disconnect(head_server) == -1) {
        printf("[Client-Hashtable] - Error disconnecting from the server\n");
        return -1;
    }

    if (rtable_disconnect(tail_server) == -1) {
        printf("[Client-Hashtable] - Error disconnecting from the server\n");
        return -1;
    }

    return 0;
}