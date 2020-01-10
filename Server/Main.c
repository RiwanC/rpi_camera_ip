#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>

int socket_RV;

void sigint_handler(int sig){
    printf("Signal caught\n");
    close(socket_RV);
    exit(0);
}

int initSocketServer(int port, struct sockaddr_in* sinp){
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
    *sinp = sin;
    return sock_RV;
}

void waitForConnection(int socket_RV, struct sockaddr_in* sin){
    socklen_t length = sizeof(*sin);
    int socket = accept(socket_RV, (struct sockaddr*) sin, &length);
    printf("Connection of client\n");
    char buf[256] = "\0";
    read(socket, buf, 255*sizeof(char));
    buf[255] = '\0';
    printf("%s is connected\n",buf);
    fflush(stdin);
    close(socket);
}

int main(int argc, char* argv[]){
    signal(SIGINT, sigint_handler);
    if (argc != 2){
        printf("Usage ./server port\n");
        exit(1);
    }
    struct sockaddr_in sin = {0};
    socket_RV = initSocketServer(atoi(argv[1]), &sin);
    while (1){
        waitForConnection(socket_RV, &sin);
    }
}
