#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>

typedef struct socketPack {
    int socket;
    struct sockaddr_in sin;
} SOCKET_PACK;

SOCKET_PACK* init_socket(int port, char* address){
    int socket_service = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_service == -1){
        perror("Unable to create the socket");
        exit(1);
    }
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    struct hostent* host, *gethostbyname();
    host = gethostbyname(address);
    bcopy(host->h_addr, &sin.sin_addr.s_addr,host->h_length);
    SOCKET_PACK* socket_pack = malloc(sizeof(SOCKET_PACK));
    socket_pack->socket = socket_service;
    socket_pack->sin = sin;
    return socket_pack;
}

int main(int argc, char* argv[]){
    if (argc != 3){
        printf("Usage: ./client address port");
        exit(1);
    }
    SOCKET_PACK* pack = init_socket(atoi(argv[2]), argv[1]);
    if (connect(pack->socket,(struct sockaddr*) &(pack->sin), sizeof(pack->sin)) != 0){
        printf("Cannot connect to server\n");
        exit(1);
    }
    char buf[256] = "\0";
    fgets(buf, 256*sizeof(char), stdin);
    printf("%s\n",buf);
    buf[255] = '\0';
    write(pack->socket, buf, 256*sizeof(char));
    exit(0);
}
