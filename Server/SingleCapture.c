//
// Created by Riwan on 07/01/2020.
//

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

uint8_t *buffer;

//check if the capture is available or not
static int xioctl(int fd, int request, void *arg)
{
    //wrapper function over ioct.ioctl() which is a function to manipulate device parameters of special files
    int r;
    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);
    return r;
}

//retrieve camera capabilities
int print_caps(int fd)
{
    struct v4l2_capability caps = {};
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &caps))
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

    if (-1 == xioctl (fd, VIDIOC_CROPCAP, &cropcap))
    {
        perror("Pb querying Cropping Capabilities");
        return 1;
    }

    printf( "Camera Cropping:\n"
            "  Bounds: %dx%d+%d+%d\n"
            "  Default: %dx%d+%d+%d\n"
            "  Aspect: %d/%d\n",
            cropcap.bounds.width, cropcap.bounds.height, cropcap.bounds.left, cropcap.bounds.top,
            cropcap.defrect.width, cropcap.defrect.height, cropcap.defrect.left, cropcap.defrect.top,
            cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);

    int support_grbg10 = 0;

    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    char fourcc[5] = {0};
    char c, e;
    printf("  FMT : CE Desc\n--------------------\n");
    while (0 == xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))
    {
        strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
        if (fmtdesc.pixelformat == V4L2_PIX_FMT_SGRBG10)
            support_grbg10 = 1;
        c = fmtdesc.flags & 1? 'C' : ' ';
        e = fmtdesc.flags & 2? 'E' : ' ';
        printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
        fmtdesc.index++;
    }

    if (!support_grbg10)
    {
        printf("Doesn't support GRBG10.\n");
        return 1;
    }

    //V4L2 provides an easy interface to check the image formats and colorspace that the webcam supports and provide. v4l2_format structure is to be used to change image format.
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //image width and height according to what the camera supports + format (mjpeg, yuv, ...)
    fmt.fmt.pix.width = 752;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SGRBG10;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        perror("Pb setting Pixel Format");
        return 1;
    }

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


int init_mmap(int fd)
{
    //A buffer contains data exchanged by application and driver using Streaming I/O methods. v4l2_requestbuffers is used to allocate device buffers.
    struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        perror("Pb requesting Buffer");
        return 1;
    }

    //After requesting buffer from the device, we need to query the buffer in order to get raw data.
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
    {
        perror("Pb querying Buffer");
        return 1;
    }

    //The mmap() function asks to map length bytes starting at offset in the memory of the device specified by fd into the application address space, preferably at address start.
    buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    printf("Length: %d\nAddress: %p\n", buf.length, buffer);
    printf("Image Length: %d\n", buf.bytesused);

    return 0;
}

//capture image
int capture_image(int fd)
{
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

    //store data
    int outfd = open("out.img", O_RDWR);
    write(outfd, buffer, buf.bytesused);
    close(outfd);

    return 0;
}

int main()
{
    int fd;
    //open the capture device. Default capture device is /dev/video0
    fd = open("/dev/video0", O_RDWR);
    if (fd == -1)
    {
        //could not find capture device
        perror("Pb opening video device");
        return 1;
    }

    //return 1 if errors occured
    if(print_caps(fd))
        return 1;

    if(init_mmap(fd))
        return 1;

    if(capture_image(fd))
        return 1;
    ////////////////////////////

    //close the capture device
    close(fd);
    return 0;
}