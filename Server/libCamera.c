#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <libv4l2.h>
#include <unistd.h>
#include <stdlib.h>
#include <jpeglib.h>

#include "libCamera.h"


#define CLEAR(x) memset(&(x), 0, sizeof (x))

// minimum number of buffers to request in VIDIOC_REQBUFS call
#define VIDIOC_REQBUFS_COUNT 2

struct buffer {
        void *                  start;
        size_t                  length;
};

static unsigned int     n_buffers       = 0;
struct buffer *         buffers         = NULL;



static char* deviceName = "/dev/video0";
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


static void deviceInit(CAMERA* myCam)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_streamparm frameint;
	unsigned int min;

	if (-1 == xioctl(myCam->fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n",deviceName);
			myCam->status = -1;
		} else {
            fprintf(stderr, "%s error %d, %s\n", "VIDIOC_QUERYCAP", errno, strerror(errno));
            myCam->status = -1;
        }
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n",deviceName);
		myCam->status = -1;
	}


		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf(stderr, "%s does not support streaming i/o\n", deviceName);
            myCam->status = -1;
        }


	/* Select video input, video standard and tune here. */
	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(myCam->fd, VIDIOC_CROPCAP, &cropcap);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */
    xioctl(myCam->fd, VIDIOC_S_CROP, &crop);

	CLEAR(fmt);

	// v4l2_format
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;

	if (-1 == xioctl(myCam->fd, VIDIOC_S_FMT, &fmt))
	    fprintf(stderr, "%s error %d, %s\n", "VIDIOC_S_FMT", errno, strerror(errno));
        myCam->status = -1;
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUV420) {
            fprintf(stderr,"Libv4l didn't accept YUV420 format. Can't proceed.\n");
            myCam->status = -1;
        }

	/* Note VIDIOC_S_FMT may change width and height. */
	if (width != fmt.fmt.pix.width) {
		width = fmt.fmt.pix.width;
		fprintf(stderr,"Image width set to %i by device %s.\n", width, deviceName);
		myCam->status = -1;
	}

	if (height != fmt.fmt.pix.height) {
		height = fmt.fmt.pix.height;
		fprintf(stderr,"Image height set to %i by device %s.\n", height, deviceName);
		myCam->status = -1;
	}

    CLEAR(frameint);

    frameint.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frameint.parm.capture.timeperframe.numerator = 1;
    frameint.parm.capture.timeperframe.denominator = 30;
    if (-1 == xioctl(myCam->fd, VIDIOC_S_PARM, &frameint)) {
        fprintf(stderr, "Unable to set frame interval.\n");
        myCam->status = -1;
    }

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;
}


static void jpegWrite(unsigned char* img, char* jpegFilename)
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
void capture_image(CAMERA* myCam)
{
    unsigned char* src;
    unsigned char* dst = malloc(width*height*3*sizeof(char));
    struct v4l2_buffer buf = {0};


    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(myCam->fd, VIDIOC_QBUF, &buf))
    {
        perror("Pb querying Buffer");
        myCam->status = -1;
    }


    //After querying the buffer, the only thing left is capturing the frame and saving it in the buffer.
    if(-1 == xioctl(myCam->fd, VIDIOC_STREAMON, &buf.type))
    {
        perror("Pb starting Capture");
        myCam->status = -1;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(myCam->fd, &fds);
    struct timeval tv = {0};
    tv.tv_sec = 2;
    int r = select(myCam->fd+1, &fds, NULL, NULL, &tv);
    if(-1 == r)
    {
        perror("Waiting for Frame");
        myCam->status = -1;
    }

    if(-1 == xioctl(myCam->fd, VIDIOC_DQBUF, &buf))
    {
        perror("Pb retrieving Frame");
        myCam->status = -1;
    }

    //store data - YUV420
/*    int outfd = open("out.img", O_WRONLY|O_CREAT);*/
/*    write(outfd, buffers, buf.bytesused);*/
/*    close(outfd);*/

    //store data - YUV440 / JPEG
    src = (unsigned char*) buffers[0].start;

    if (myCam->status != -1){
        YUV420toYUV444(width, height, src, dst);
        myCam->lastImage = dst;
    }
    //jpegWrite(dst, "image.jpg");
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





static void mmapInit(CAMERA* myCam)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = VIDIOC_REQBUFS_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(myCam->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", deviceName);
            myCam->status = -1;
        } else {
            fprintf(stderr, "%s error %d, %s\n", "VIDIOC_REQBUFS", errno, strerror(errno));
            myCam->status = -1;
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", deviceName);
        myCam->status = -1;
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        myCam->status = -1;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl(myCam->fd, VIDIOC_QUERYBUF, &buf)) {
            fprintf(stderr, "%s error %d, %s\n", "VIDIOC_QUERYBUF", errno, strerror(errno));
            myCam->status = -1;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL /* start anywhere */, buf.length,
                                             PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, myCam->fd,
                                             buf.m.offset);
        printf("Length: %d\nAddress: %p\n", buf.length, buffers);
        printf("Image Length: %d\n", buf.bytesused);


        if (MAP_FAILED == buffers[n_buffers].start){
            fprintf(stderr, "%s error %d, %s\n", "mmap", errno, strerror(errno));
            myCam->status = -1;
        }
    }
}

void initCamera(CAMERA* myCam){
    struct stat st;
    myCam->status = 0;

    if (-1 == stat(deviceName, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", deviceName, errno, strerror(errno));
        myCam->status = -1;
    }
    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", deviceName);
        myCam->status = -1;
    }
    myCam->fd = v4l2_open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (myCam->fd == -1)
    {
        perror("Pb opening video device");
        myCam->status = -1;
    }
    deviceInit(myCam);
    mmapInit(myCam);
}
