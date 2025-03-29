/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include "table.h"  
#include "server_hashtable.h"
#include <zookeeper/zookeeper.h>           
#include "server_network.h"
#include "server_network_private.h"
#include "stats.h"
#include "server_skeleton.h"
#include "client_stub.h"

// ZooKeeper Constants
static char *watcher_ctx = "ZooKeeper Data Watcher";
static char *ROOT_PATH = "/chain";
static char *NODE_PATH = "/chain/node";
static size_t PATH_LEN = 512;
// ZooKeeper Variables
zhandle_t* zh;
int is_connected_to_zookeeper = 0;
char* current_server_node_path;
// Next Server Information
struct rtable_t* next_server;
char* next_server_node_path;
// Local Table
struct table_t *table;
// Server Network Variables
int server_socket;
extern int global_terminate;
// Other Constants
static size_t IP_LEN = 120;

/** Inicializar a table_t local e o Server Network
*/
int init_server(short server_port, int n_lists) {
    if (server_port < 1024) {
        printf("[Server-Hashtable] - O número de Porto deve ser >= 1024\n");
        return -1;
    }
    if (n_lists <= 0) {
        printf("[Server-Hashtable] - O número de listas deve ser positivo\n");
        return -1;
    }
    table = server_skeleton_init(n_lists);
    if (table == NULL) {
        printf("[Server-Hashtable] - Erro a inicializar a Tabela Local\n");
        return -1;
    }
    server_socket = server_network_init(server_port);
    if (server_socket == -1) {
        printf("[Server-Hashtable] - Erro a inicializar a Socket do Servidor\n");
        return -1;
    }
    return 0;
}

int connect_to_zookeeper(char* zookeeper_port) {
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    // Inicializar o Servidor ZooKeeper
	zh = zookeeper_init(zookeeper_port,connection_watcher,2000,0,0,0); 
	if (zh == NULL)	{
		printf("[Server-Hashtable] - Erro a conectar ao Servidor ZooKeeper\n");
        return -1;
	}
    return 0;
}

int create_server_node(char* server_port) {
    // Esperar até ser possível conectar ao Zookeeper
    while(is_connected_to_zookeeper);
    // Verificar se o node /chain já foi criado
    if(zoo_exists(zh,ROOT_PATH, 0, NULL) == ZNONODE){ 
        if (ZOK != zoo_create(zh,ROOT_PATH,NULL,-1, & ZOO_OPEN_ACL_UNSAFE,0,NULL,0)) {
            printf("[Server-Hashtable] - Erro a criar o node /chain no ZooKeeper\n");
            return -1;
        }
        printf("[Server-Hashtable] - Node /chain criado no Servidor ZooKeeper\n");
    }
    // Obter o <IP:Port> do Servidor Atual
    char* full_server_ip = get_full_server_address(server_port);
    char* node_path = malloc(PATH_LEN);
    memset(node_path, 0, PATH_LEN);
    // Criar o Node do Servidor Atual
    if(ZOK != zoo_create(zh,NODE_PATH,full_server_ip,strlen(full_server_ip)+1,&ZOO_OPEN_ACL_UNSAFE,ZOO_EPHEMERAL | ZOO_SEQUENCE,node_path,PATH_LEN)){
        printf("[Server-Hashtable] - Erro a criar o node no Servidor ZooKeeper\n");
        return -1;
    }
    current_server_node_path = node_path;
    return 0;
}

int synchronize_server_zookeeper() {
    // Obter uma lista dos Servidores Ativos
    zoo_string* children_list = malloc(sizeof(zoo_string));
    if (ZOK != zoo_wget_children(zh,ROOT_PATH,&child_watcher,watcher_ctx,children_list) || children_list == NULL || children_list->count == 0) {
        printf("[Server-Hashtable] - Não existem Servidores Ativos\n");
        return -1;
    }
    // O único Servidor Ativo é o atual
    if (children_list->count == 1) {
        printf("[Server-Hashtable] - O Servidor atual é o único ligado ao Servidor ZooKeeper\n");
        next_server = NULL;
        return 0;
    }
    // Obter o Index do Next Server
    int next_server_index = get_next_server_index(children_list);
    if (next_server_index == -1) {
        // Se o Servidor Atual for o último, então não tem next_server
        if (next_server != NULL) {
            unregister_client();
            rtable_disconnect(next_server);
        }
        next_server = NULL;
        next_server_node_path = NULL;
    }
    else {
        // Atualizar o next_server, estabelecendo uma conexão ao mesmo
        if (update_next_server(children_list->data[next_server_index]) == -1) {
            printf("[Server-Hashtable] - Não foi possível estabelecer uma ligação ao Next Server com PATH=%s\n",children_list->data[next_server_index]);
            return -1;
        }
    }
    // Obter o Index do Previous Server
    int previous_server_index = get_previous_server_index(children_list);
    if (previous_server_index == -1) {
        // Se o Servidor Atual for o primeiro, então não tem previous_server
        printf("[Server-Hashtable] - O Servidor Atual é o primeiro, portanto não se atualizou a tabela local\n");
    }
    else {
        // Atualizar a tabela local do servidor atual, tendo por base a tabela do previous server
        if (update_local_table_by_server(children_list->data[previous_server_index]) == -1) {
            printf("[Server-Hashtable] - Não foi possível atualizar a tabela local com base no Previous Server com PATH=%s\n",children_list->data[next_server_index]);
            return -1;
        }
        register_client();
    }
    return 0;
}

int update_next_server(char* n_server) {
    char* n_server_path = malloc(PATH_LEN);
    if (n_server_path == NULL) {
        printf("[Server-Hashtable] - Não foi possível alocar memória para o Path do Next Server\n");
        return -1;
    }
    // Inicializar n_server_path a ""
    memset(n_server_path,0,PATH_LEN);
    // Obter o Full Path do Next Server
    strcat(n_server_path,ROOT_PATH);
    strcat(n_server_path,"/");
    strcat(n_server_path,n_server);
    // Obter o Endereço IP do Servidor
    char n_server_address[1024];
    int n_server_ip_size = sizeof(n_server_address);
    if (ZOK != zoo_get(zh,n_server_path,0,n_server_address,&n_server_ip_size,NULL)) {
        printf("[Server-Hashtable] - Não foi possível obter o Endereço IP do Next Server\n");
        free(n_server_path);
        return -1;
    }
    // Estabelecer a ligação ao Next Server
    next_server = rtable_connect(n_server_address);
    if (next_server == NULL) {
        printf("[Server-Hashtable] - Não foi possível estabelecer a ligação ao Next Server\n");
        free(n_server_path);
        return -1;
    }
    next_server_node_path = n_server_path;
    printf("[Server-Hashtable] - Novo Next Server IP=%s PATH=%s\n",n_server_address,next_server_node_path);
    return 0;
}

int update_local_table_by_server(char* p_server) {
    // Obter o Full Path do Previous Server
    char* p_server_path = malloc(PATH_LEN);
    memset(p_server_path,0,PATH_LEN);
    strcat(p_server_path,ROOT_PATH);
    strcat(p_server_path,"/");
    strcat(p_server_path,p_server);
    // Obter o Endereço IP do Servidor
    char p_server_address[512];
    int p_server_ip_size = sizeof(p_server_address);
    if (ZOK != zoo_get(zh,p_server_path,0,p_server_address,&p_server_ip_size,NULL)) {
        printf("[Server-Hashtable] - Não foi possível obter o Endereço IP do Previous Server\n");
        return -1;
    }
    // Estabelecer uma ligação ao Servidor Anterior
    struct rtable_t* previous_server = rtable_connect(p_server_address);
    if (previous_server == NULL){
        printf("[Server-Hashtable] - Não foi possivel estabelecer uma ligação ao Servidor Anterior\n");
        free(p_server_path);
        return -1;
    }
    // Atualizar a tabela local consoante a tabela do Servidor Anterior
    struct entry_t** p_server_table = rtable_get_table(previous_server);   
    for (int i = 0; i < rtable_size(previous_server); i++) {
        // Colocar cada entrada da tabela do Servidor Anterior na Tabela Local
        if(table_put(table,p_server_table[i]->key,p_server_table[i]->value) == -1){
            printf("[Server-Hashtable] - Ocorreu um erro a atualizar a tabela local\n");
            rtable_free_entries(p_server_table);
            rtable_disconnect(previous_server);
            free(p_server_path);
            return -1;
        }
    }
    rtable_free_entries(p_server_table);
    // Desligar a ligação ao Servidor Anterior
    if (rtable_disconnect(previous_server) == -1) {
        printf("[Server-Hashtable] - Ocorreu um erro a desligar a ligação ao Servidor Anterior\n");
        free(p_server_path);
        return -1;
    }
    free(p_server_path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: <porto_servidor> <n_lists> <ip:porto>\n");
        return -1;
    }
    char* server_port = argv[1];
    int n_lists = atoi(argv[2]);
    char* zookeeper_port = argv[3];
    // Inicializar a estrutura Table local e a Socket do Servidor
    if(init_server((short) atoi(server_port), n_lists) == -1) {
        return -1;
    }
    // Connectar ao ZooKeeper
    if (connect_to_zookeeper(zookeeper_port) == -1) {
        return -1;
    }
    // Criar o node /chain se não existir e criar o node do Servidor
    if (create_server_node(server_port) == -1) {
        return -1;
    }
    // Definir o next server e atualizar a tabela local consoante o que estiver no ZooKeeper
    if (synchronize_server_zookeeper() == -1) {
        return -1;
    }
    printf("Servidor pronto e a ouvir o porto %d com %d listas\n", (int) atoi(server_port), n_lists);
    signal(SIGINT, stop_execution);  
    if (network_main_loop(server_socket, table) == -1) {
        printf("Erro no loop de processamento do servidor\n");
    }
    if (server_skeleton_destroy(table) == -1) {
        printf("Erro ao destruir a tabela.\n");
        return -1;
    }
    return 0;
}

/* Para a execução do servidor, libertando toda a memória e recursos alocados.
*/
void stop_execution(int signal) {
    global_terminate = 1;
    server_network_close(server_socket);
    close_zookeeper_connection();
    destroy_rwlock();
    printf("Servidor encerrado\n");
    exit(0);
}

void close_zookeeper_connection() {
    if (current_server_node_path) {
        free(current_server_node_path);
    }
    if(next_server_node_path != NULL){
        free(next_server_node_path);
    }
    if(next_server != NULL){
        rtable_disconnect(next_server);
    }
    zookeeper_close(zh);
    return;
}

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected_to_zookeeper = 1; 
		} else {
			is_connected_to_zookeeper = 0; 
		}
	}
}

/**
* Data Watcher function for /MyData node
*/
void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    if (state != ZOO_CONNECTED_STATE || type != ZOO_CHILD_EVENT) {
        return;
    }
    // Obter a lista de Servidores Ativos
    zoo_string* children_list = malloc(sizeof(zoo_string));
    if (ZOK != zoo_wget_children(zh, ROOT_PATH, &child_watcher, watcher_ctx, children_list) || children_list->count == 0) {
        printf("[Server-Hashtable] - Não existem Servidores Ativos\n");
        return;
    }
    if (children_list->count == 1) {
        printf("[Server-Hashtable] - O único Servidor ativo é o atual\n");
        return;
    }
    // Obter o indice do Next Server dentro dos Servidores Ativos
    int next_server_index = get_next_server_index(children_list);
    // Se o Servidor Atual for o último, então não tem next_server
    if (next_server_index == -1) {
        next_server = NULL;
        next_server_node_path = NULL;
        return;
    }
    // Obter o ID do Next Server
    int nserver_id = atoi(strtok(children_list->data[next_server_index],"node"));
    // Se o Servidor não estava conectado a um Next Server ou estava e conectou-se a um diferente
    if (next_server == NULL || get_id(next_server_node_path) != nserver_id) {
        // Desconectar do Servidor Anterior
        if (next_server != NULL) {
            unregister_client();
            rtable_disconnect(next_server);
            next_server_node_path = NULL;
            next_server = NULL;
        }
        // Atualizar o Next Server
        update_next_server(children_list->data[next_server_index]);
        register_client();
    }
    // Se o Next Server se mantiver igual não é preciso fazer nada
    return;
}

// Funções Auxiliares

/*
* Retorna o ID do Servidor dado
*/
int get_id(char* server_path) {
    char* id = strtok(server_path,NODE_PATH);
    return atoi(id);
}

char* get_full_server_address(char* server_port) {
    char* full_address = malloc(IP_LEN);
    strcpy(full_address,"");
    char* local_address = get_local_ip_address(server_socket);
    strcat(full_address,local_address);
    strcat(full_address,":");
    strcat(full_address,server_port);
    return full_address;
}

/*
* Retorna o Index do Servidor logo a seguir ao atual
* > Requer que children_list->count > 1
*/
int get_next_server_index(zoo_string* children_list) {
    int server_id = get_id(current_server_node_path);
    int next_server_id = INT_MAX;
    int next_server_index = -1;
    for (int i = 0; i < children_list->count; i++) {
        int current_id = get_id(children_list->data[i]);
        if (current_id > server_id && current_id < next_server_id) {
            next_server_id = current_id;
            next_server_index = i;
        }
    }
    return next_server_index;
}

/*
* Retorna o Index do Servidor logo atras do atual
* > Requer que children_list->count > 1
*/
int get_previous_server_index(zoo_string* children_list) {
    int server_id = get_id(current_server_node_path);
    int previous_server_id = INT_MIN;
    int previous_server_index = -1;
    for (int i = 0; i < children_list->count; i++) {
        int current_id = get_id(children_list->data[i]);
        if (current_id < server_id && current_id > previous_server_id) {
            previous_server_id = current_id;
            previous_server_index = i;
        }
    }
    return previous_server_index;
}
