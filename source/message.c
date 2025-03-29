/*
* Grupo SD-41
* - Miguel Mendes fc59885
* - Pedro Correia fc59791
* - Sara Ribeiro fc59873
*/

#include "message-private.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int write_all(int sock, void *buf, int len) {
    int bufsize = len;
    while(len>0) {
        int res = write(sock, buf, len);
        if(res<0) {
            if(errno == EINTR) continue;
            perror("write failed:");
            return res; /* Error != EINTR */
        }
        if(res==0) return res; /* Socket was closed */
        buf += res;
        len -= res;
    }
    return bufsize;
}

int read_all(int sock, void *buf, int len) {
    int bufsize = len;
    while(len>0) {
        int res = read(sock, buf, len);
        if(res<0) {
            if(errno==EINTR) continue;
            perror("write failed:");
            return res; /* Error != EINTR */
        }
        if(res==0) return res; /* Socket was closed */
        buf += res;
        len -= res;
    }
    return bufsize;
}
