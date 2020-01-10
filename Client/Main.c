#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

int socket_service;

void sigint_handler(int sig){
    printf("Signal caught\n");
    close(socket_service);
    exit(0);
}

int init_socket(int port, char* address, struct sockaddr_in* sinp){
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
    *sinp = sin;
    return socket_service;
}

int main(int argc, char* argv[]){
    signal(SIGINT, sigint_handler);
    if (argc != 3){
        printf("Usage: ./client address port");
        exit(1);
    }
    struct sockaddr_in sin = {0};
    socket_service = init_socket(atoi(argv[2]), argv[1], &sin);
    if (connect(socket_service,(struct sockaddr*) &(sin), sizeof(sin)) != 0){
        printf("Cannot connect to server\n");
        exit(1);
    }
    char buf[256] = "\0";
    printf("Type your name to send it to the server\n");
    fgets(buf, 256*sizeof(char), stdin);
    for (int i=0;i<256;i++){
        if (buf[i] == '\n')buf[i] = '\0';
    }
    buf[255] = '\0';
    write(socket_service, buf, 256*sizeof(char));
    exit(0);
}
