/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include "client_network.h"
#include "client_stub-private.h"
#include "client_stub.h"
#include "htmessages.pb-c.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "message.c"
#include "message-private.h"

//#include "simple.pb-c.h" // n devia ser o .c?

/* Esta função deve:
* - Obter o endereço do servidor (struct sockaddr_in) com base na
* informação guardada na estrutura rtable;
* - Estabelecer a ligação com o servidor;
* - Guardar toda a informação necessária (e.g., descritor do socket)
* na estrutura rtable;
* - Retornar 0 (OK) ou -1 (erro).
*/
int network_connect(struct rtable_t *rtable) {
    
    struct sockaddr_in server;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0 ) {
        perror("[Client_Network] - Erro a criar a socket\n");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = rtable->server_port;

    if (inet_pton(AF_INET,rtable->server_address,&server.sin_addr) < 1) {
        printf("[Client_Network] - Erro a converter o endereço\n");
        return -1;
    }

    if (connect(sockfd,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("[Client_Network] - Erro a estabelecer ligação com o servidor\n");
        return -1;
    }

    rtable->sockfd = sockfd;

    return 0;
}

/* Esta função deve:
* - Obter o descritor da ligação (socket) da estrutura rtable_t;
* - Serializar a mensagem contida em msg;
* - Enviar a mensagem serializada para o servidor;
* - Esperar a resposta do servidor;
* - De-serializar a mensagem de resposta;
* - Tratar de forma apropriada erros de comunicação;
* - Retornar a mensagem de-serializada ou NULL em caso de erro.
*/
MessageT *network_send_receive(struct rtable_t *rtable, MessageT *msg) {

    signal(SIGPIPE, SIG_IGN);

    short buffer_size = (short) message_t__get_packed_size(msg);
    void* buf = malloc(buffer_size);

    if (buf == NULL) {
        perror("[Client-Network] - Erro a alocar memória para o buffer a receber do servidor");
        return NULL;
    }

    message_t__pack(msg,buf);

    // Receber o tamanho do buffer
    if(write_all(rtable->sockfd,&buffer_size,sizeof(short)) != sizeof(short)){
        perror("[Client-Network] - Erro a enviar a dimensão do buffer ao servidor\n");
        free(buf);
        return NULL;
    }
    // Enviar o buffer
    if(write_all(rtable->sockfd,buf,buffer_size) != buffer_size){
        perror("[Client-Network] - Erro a enviar o buffer ao servidor\n");
        free(buf);
        return NULL;
    }

    free(buf);

    short received_buffer_size;
    // Receber o tamanho do buffer do servidor
    if(read_all(rtable->sockfd,&received_buffer_size,sizeof(short)) != sizeof(short)){
        perror("[Client-Network] - Erro a receber o tamanho do buffer do servidor\n");
        return NULL;
    }

    buf = malloc(received_buffer_size);

    if (buf == NULL) {
        perror("[Client-Network] - Erro a alocar memória para o buffer a receber do servidor");
        return NULL;
    }

    if (read_all(rtable->sockfd,buf,received_buffer_size) != received_buffer_size) {
        perror("[Client-Network] - Erro a receber o buffer do servidor\n");
        free(buf);
        return NULL;
    }
    
    MessageT *received_msg = message_t__unpack(NULL,received_buffer_size,buf);
    free(buf);
    if (received_msg == NULL) {
        perror("[Client-Network] - Erro a de-serializar a mensagem recebida \n");
        return NULL;
    }

    return received_msg;
}

/* Fecha a ligação estabelecida por network_connect().
* Retorna 0 (OK) ou -1 (erro).
*/
int network_close(struct rtable_t *rtable) {
    if(rtable == NULL) {
        perror("[Client-Network] - Argumento inválido\n");
        return -1;
    }

    int result = close(rtable->sockfd);
    if(result < 0) {
        perror("[Client-Network] - Erro a fechar o socket\n");
        return -1;
    }

    return 0;
}
