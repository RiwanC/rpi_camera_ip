#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "libCamera.h"

#define SNAPSHOT 0
#define CLOSE 1


typedef struct threadarg{
    int socket;
} THREAD_ARG;

int socket_RV;
CAMERA myCam = {0};

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

void* clientRoutine(void* arg){
    THREAD_ARG* threadArg = (THREAD_ARG*) arg;
    int command;
    while (1){
        printf("Waiting for order\n");
        command = -1;
        read(threadArg->socket, &command, sizeof(int));
        switch (command){
            case SNAPSHOT:
                printf("Received snapshot order\n");
                capture_image(&myCam);
                printf("Image taken with status %d\n", myCam.status);
                write(threadArg->socket, &myCam.status, sizeof(int));
                if (myCam.status == 0){
                    send(threadArg->socket, myCam.lastImage, sizeof(char)*width*height*3,0);
                }
                break;
            case CLOSE:
                break;
        }
    }
    return NULL;
}

void sendStatus(int socket){
    write(socket, &(myCam.status), sizeof(int));

}

void waitForConnection(int socket_RV, struct sockaddr_in* sin){
    socklen_t length = sizeof(*sin);
    pthread_t clientThread;
    THREAD_ARG threadArg;
    int socket = accept(socket_RV, (struct sockaddr*) sin, &length);
    printf("Connection of client\n");
    threadArg.socket = socket;
    sendStatus(socket);
    pthread_create(&clientThread, NULL, clientRoutine, (void*) &threadArg);
}

int main(int argc, char* argv[]){
    signal(SIGINT, sigint_handler);
    if (argc != 2){
        printf("Usage ./server port\n");
        exit(1);
    }
    initCamera(&myCam);
    printf("Camera status: %d\n", myCam.status);
    struct sockaddr_in sin = {0};
    socket_RV = initSocketServer(atoi(argv[1]), &sin);
    while (1){
        waitForConnection(socket_RV, &sin);
    }
}
