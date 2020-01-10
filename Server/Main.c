#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>

typedef struct socketPack {
    int socket;
    struct sockaddr_in sin;
} SOCKET_PACK;

SOCKET_PACK* initSocketServer(int port){
    int sock_RV = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_RV == -1){
        perror("Unable to create the socket");
        exit(1);
    }
    struct sockaddr_in sin;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (bind(sock_RV, (struct sockaddr*) &sin, sizeof(sin)) == -1){
        perror("Unable to bind socket");
        exit(1);
    }
    if (listen(sock_RV,5) == -1){
        perror("Unable to listen on the socket");
        exit(1);
    }
    SOCKET_PACK* pack = malloc(sizeof(SOCKET_PACK));
    pack->socket = sock_RV;
    pack->sin = sin;
    return pack;
}

void waitForConnection(SOCKET_PACK* packRV){
    socklen_t length = sizeof(packRV->sin);
    int socket = accept(packRV->socket, (struct sockaddr*) &(packRV->sin), &length);
    printf("Connection of client\n");
    char buf[256] = "\0";
    read(socket, buf, 255*sizeof(char));
    buf[255] = '\0';
    printf("%s is connected",buf);
    fflush(stdin);
}

int main(int argc, char* argv[]){
    if (argc != 2){
        printf("Usage ./server port\n");
        exit(1);
    }
    SOCKET_PACK* packRV = initSocketServer(atoi(argv[1]));
    while (1){
        waitForConnection(packRV);
    }
}
