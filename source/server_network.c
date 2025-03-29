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
#include <time.h>
#include "server_network_private.h"
#include "message-private.h"
#include "server_network.h"
#include "server_skeleton.h"
#include "client_network.h"
#include "client_stub-private.h"
#include "client_stub.h"
#include "stats.h"

#define MAX_SOCKETS 20

int global_server_socket;
int global_terminate = 0;
pthread_t socket_threads[MAX_SOCKETS];

/* Função para preparar um socket de receção de pedidos de ligação
* num determinado porto.
* Retorna o descritor do socket ou -1 em caso de erro.
*/
int server_network_init(short port) {
    struct sockaddr_in server;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("[Server-Network] - Erro a criar socket\n");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("[Server-Network] - Erro a associar porto\n");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 0) < 0){
        perror("[Server-Network] - Erro ao executar listen\n");
        close(sockfd);
        return -1;
    };

    int opt = 1;
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt, sizeof(opt)) == -1) {
        perror("[Server-Network] - Erro ao permitir a reutilização de endereços\n");
        close(sockfd);
        return -1;
    }

    printf("[Server-Network] - Servidor a executar no Porto %d.\n", port);
    return sockfd;
}

/* A função network_main_loop() deve:
* - Aceitar uma conexão de um cliente;
* - Receber uma mensagem usando a função network_receive;
* - Entregar a mensagem de-serializada ao skeleton para ser processada
na tabela table;
* - Esperar a resposta do skeleton;
* - Enviar a resposta ao cliente usando a função network_send.
* A função não deve retornar, a menos que ocorra algum erro. Nesse
* caso retorna -1.
*/
int network_main_loop(int listening_socket, struct table_t *table) {
    struct sockaddr_in client;
    socklen_t size_client = sizeof(client);
    global_server_socket = listening_socket;
    while (global_terminate != 1) {
        int connected_socket = accept(listening_socket,(struct sockaddr *) &client, &size_client);
        if (connected_socket == -1) { continue; }
        else {
            create_new_thread(connected_socket, table, socket_threads);
        }
    }
    return 0;
}

/* A função network_receive() deve:
* - Ler os bytes da rede, a partir do client_socket indicado;
* - De-serializar estes bytes e construir a mensagem com o pedido,
* reservando a memória necessária para a estrutura MessageT.
* Retorna a mensagem com o pedido ou NULL em caso de erro.
*/
MessageT *network_receive(int client_socket) {
    short received_buffer_size;

    if(read_all(client_socket,&received_buffer_size,sizeof(short)) != sizeof(short)){
        perror("[Server-Network] - Erro a receber o tamanho do buffer do cliente\n");
        return NULL;
    };

    void* buf = malloc(received_buffer_size);

    if (buf == NULL) {
        perror("[Server-Network] - Erro a alocar memória para o buffer a receber do cliente");
        return NULL;
    }

    if (read_all(client_socket,buf,received_buffer_size) != received_buffer_size) {
        perror("[Server-Network] - Erro a receber o buffer do cliente\n");
        free(buf);
        return NULL;
    }

    MessageT *received_msg = message_t__unpack(NULL,received_buffer_size,buf);
    if (received_msg == NULL) {
        perror("[Server-Network] - Erro a de-serializar a mensagem recebida \n");
        free(buf);
        return NULL;
    }

    free(buf);
    return received_msg;
}

/* A função network_send() deve:
* - Serializar a mensagem de resposta contida em msg;
* - Enviar a mensagem serializada, através do client_socket.
* Retorna 0 (OK) ou -1 em caso de erro.
*/
int network_send(int client_socket, MessageT *msg) {
    if (msg == NULL || msg->base.descriptor != &message_t__descriptor) {
        perror("[Server-Network] - MessageT inválida");
        return -1;
    }

    short buffer_size = (short) message_t__get_packed_size(msg);
    void* buf = malloc(buffer_size);

    if (buf == NULL) {
        perror("[Server-Network] - Erro a alocar memória para o buffer a receber do cliente");
        return -1;
    }

    message_t__pack(msg,buf);

    // Receber o tamanho do buffer
    if(write_all(client_socket,&buffer_size,sizeof(short)) != sizeof(short)){
        perror("[Server-Network] - Erro a enviar a dimensão do buffer ao cliente\n");
        free(buf);
        return -1;
    }
    // Enviar o buffer
    if(write_all(client_socket,buf,buffer_size) != buffer_size){
        perror("[Server-Network] - Erro a enviar o buffer ao cliente\n");
        free(buf);
        return -1;
    }

    free(buf);
    return 0;
}

/* Liberta os recursos alocados por server_network_init(), nomeadamente
* fechando o socket passado como argumento.
* Retorna 0 (OK) ou -1 em caso de erro.
*/
int server_network_close(int socket) {
    int result = close(socket);
    if(result < 0) {
        perror("[Server-Network] - Erro a fechar o socket\n");
        return -1;
    }
    return 0;
}
