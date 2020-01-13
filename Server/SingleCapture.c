
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libv4l2.h>
#include <unistd.h>
#include <stdlib.h>
#include <jpeglib.h>



#define CLEAR(x) memset (&(x), 0, sizeof (x))

// minimum number of buffers to request in VIDIOC_REQBUFS call
#define VIDIOC_REQBUFS_COUNT 2

struct buffer {
        void *                  start;
        size_t                  length;
};

static unsigned int     n_buffers       = 0;
struct buffer *         buffers         = NULL;



static char* deviceName = "/dev/video0";
static unsigned int width = 640;
static unsigned int height = 480;
static unsigned char jpegQuality = 120;



/* Fonction from yuv.c */
void YUV420toYUV444(int width, int height, unsigned char* src, unsigned char* dst) {
    int line, column;
    unsigned char *py, *pu, *pv;
    unsigned char *tmp = dst;

    // In this format each four bytes is two pixels. Each four bytes is two Y's, a Cb and a Cr.
    // Each Y goes to one of the pixels, and the Cb and Cr belong to both pixels.
    unsigned char *base_py = src;
    unsigned char *base_pu = src+(height*width);
    unsigned char *base_pv = src+(height*width)+(height*width)/4;

    for (line = 0; line < height; ++line) {
        for (column = 0; column < width; ++column) {
            py = base_py+(line*width)+column;
            pu = base_pu+(line/2*width/2)+column/2;
            pv = base_pv+(line/2*width/2)+column/2;

            *tmp++ = *py;
            *tmp++ = *pu;
            *tmp++ = *pv;
        }
    }
}



static void errno_exit(const char* s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}




//check if the capture is available or not
static int xioctl(int fd, int request, void *arg)
{
    //wrapper function over ioct.ioctl() which is a function to manipulate device parameters of special files
    int r;
    do{
         r = ioctl (fd, request, arg);
    }
    while (-1 == r && EINTR == errno);
    return r;
}





static void jpegWrite(unsigned char* img, char* jpegFilename)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];
    FILE *outfile = fopen( jpegFilename, "wb" );

    // try to open file for saving
    if (!outfile) {
        errno_exit("jpeg");
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
    jpeg_set_quality(&cinfo, jpegQuality, TRUE);

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





//capture image
int capture_image(int fd)
{
    unsigned char* src;
    unsigned char* dst = malloc(width*height*3*sizeof(char));
    struct v4l2_buffer buf = {0};


    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
    {
        perror("Pb querying Buffer");
        return 1;
    }


    //After querying the buffer, the only thing left is capturing the frame and saving it in the buffer.
    if(-1 == xioctl(fd, VIDIOC_STREAMON, &buf.type))
    {
        perror("Pb starting Capture");
        return 1;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = {0};
    tv.tv_sec = 2;
    int r = select(fd+1, &fds, NULL, NULL, &tv);
    if(-1 == r)
    {
        perror("Waiting for Frame");
        return 1;
    }

    if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
    {
        perror("Pb retrieving Frame");
        return 1;
    }

    //store data - YUV420
/*    int outfd = open("out.img", O_WRONLY|O_CREAT);*/
/*    write(outfd, buffers, buf.bytesused);*/
/*    close(outfd);*/

    //store data - YUV440 / JPEG
    src = (unsigned char*) buffers[0].start;


    YUV420toYUV444(width, height, src, dst);

    jpegWrite(dst, "image.jpg");

    return 0;
}






//retrieve camera capabilities
int print_caps(int fd)
{
    struct v4l2_capability caps = {};
    if (-2 == xioctl(fd, VIDIOC_QUERYCAP, &caps))
    {
        perror("Pb querying Capabilities");
        return 1;
    }

    printf( "Driver Caps:\n"
            "  Driver: \"%s\"\n"
            "  Card: \"%s\"\n"
            "  Bus: \"%s\"\n"
            "  Version: %d.%d\n"
            "  Capabilities: %08x\n",
            caps.driver,
            caps.card,
            caps.bus_info,
            (caps.version>>16)&&0xff,
            (caps.version>>24)&&0xff,
            caps.capabilities);


    struct v4l2_cropcap cropcap = {0};
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {

    }

    printf( "Camera Cropping:\n"
            "  Bounds: %dx%d+%d+%d\n"
            "  Default: %dx%d+%d+%d\n"
            "  Aspect: %d/%d\n",
            cropcap.bounds.width, cropcap.bounds.height, cropcap.bounds.left, cropcap.bounds.top,
            cropcap.defrect.width, cropcap.defrect.height, cropcap.defrect.left, cropcap.defrect.top,
            cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);


    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    char fourcc[5] = {0};
    char c, e;
    printf("  FMT : CE Desc\n--------------------\n");
    while (0 == xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))
    {
        strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
        c = fmtdesc.flags & 1? 'C' : ' ';
        e = fmtdesc.flags & 2? 'E' : ' ';
        printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
        fmtdesc.index++;
    }

    //V4L2 provides an easy interface to check the image formats and colorspace that the webcam supports and provide. v4l2_format structure is to be used to change image format.
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //image width and height according to what the camera supports + format (mjpeg, yuv, ...)
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;


    strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
    printf( "Selected Camera Mode:\n"
            "  Width: %d\n"
            "  Height: %d\n"
            "  PixFmt: %s\n"
            "  Field: %d\n",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fourcc,
            fmt.fmt.pix.field);

    return 0;
}





static void mmapInit(int fd)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = VIDIOC_REQBUFS_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", deviceName);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", deviceName);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL /* start anywhere */, buf.length, PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, fd, buf.m.offset);
        printf("Length: %d\nAddress: %p\n", buf.length, buffers);
        printf("Image Length: %d\n", buf.bytesused);


        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}





int main()
{

    //open the capture device. Default capture device is /dev/video0
    int fd = v4l2_open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);
/*    fd = open("/dev/video0", O_RDWR);*/
    if (fd == -1)
    {
        //could not find capture device
        perror("Pb opening video device");
        return 1;
    }

    //return 1 if errors occured
    if(print_caps(fd))
        return 1;

    mmapInit(fd);


    if(capture_image(fd))
        return 1;

    //close the capture device
    close(fd);
    return 0;
}
