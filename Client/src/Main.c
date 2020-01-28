// To compile:
//      
//          gcc Main.c -o client -ljpeg
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <jpeglib.h>
#include <signal.h>
#include <errno.h>

int socket_service;
static unsigned int width = 640;
static unsigned int height = 480;

void sigint_handler(int sig){
    printf("Signal caught\n");
    close(socket_service);
    exit(0);
}

void jpegWrite(unsigned char* img, char* jpegFilename)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];
    FILE *outfile = fopen( jpegFilename, "wb" );

    // try to open file for saving
    if (!outfile) {
        fprintf(stderr, "%s error %d, %s\n", "jpeg", errno, strerror(errno));
    }

    // create jpeg data
    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    // set image parameters
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;

    // set jpeg compression parameters to default
    jpeg_set_defaults(&cinfo);
    // and then adjust quality setting
    jpeg_set_quality(&cinfo, 120, TRUE);

    // start compress
    jpeg_start_compress(&cinfo, TRUE);

    // feed data
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // finish compression
    jpeg_finish_compress(&cinfo);

    // destroy jpeg data
    jpeg_destroy_compress(&cinfo);

    // close output file
    fclose(outfile);
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
    int status = -42;
    read(socket_service, &status, sizeof(int));
    printf("Camera status: %d\n", status);
    int command = 0;
    write(socket_service, &command, sizeof(int));
    printf("Sending snapshot order\n");
    read(socket_service, &status, sizeof(int));
    printf("Camera status: %d\n", status);
    unsigned char* image = malloc(sizeof(char)*height*width*3);
    if (status != -1){
        // MSG_WAITALL should block until all data has been received. From the manual page on recv.
        int a= recv(socket_service, image, sizeof(char)*height*width*3,MSG_WAITALL);
        jpegWrite(image, "image2.jpg");
    }
    free(image);
    exit(0);
}
