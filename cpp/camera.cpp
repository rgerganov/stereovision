#include "camera.h"
#include "jpeg_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <jpeglib.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static unsigned const char jpeg_tables[] =
{
    0xff,0xd8,0xff,0xdb,0x00,0x43,0x00,0x08,0x06,0x06,0x07,0x06,0x05,0x08,0x07,0x07,
    0x07,0x09,0x09,0x08,0x0a,0x0c,0x14,0x0d,0x0c,0x0b,0x0b,0x0c,0x19,0x12,0x13,0x0f,
    0x14,0x1d,0x1a,0x1f,0x1e,0x1d,0x1a,0x1c,0x1c,0x20,0x24,0x2e,0x27,0x20,0x22,0x2c,
    0x23,0x1c,0x1c,0x28,0x37,0x29,0x2c,0x30,0x31,0x34,0x34,0x34,0x1f,0x27,0x39,0x3d,
    0x38,0x32,0x3c,0x2e,0x33,0x34,0x32,0xff,0xdb,0x00,0x43,0x01,0x09,0x09,0x09,0x0c,
    0x0b,0x0c,0x18,0x0d,0x0d,0x18,0x32,0x21,0x1c,0x21,0x32,0x32,0x32,0x32,0x32,0x32,
    0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
    0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,
    0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0xff,0xc4,0x00,0x1f,
    0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0xff,0xc4,0x00,
    0xb5,0x10,0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,
    0x01,0x7d,0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,
    0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,
    0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,
    0x27,0x28,0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,
    0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,
    0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,
    0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,
    0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
    0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,
    0xf7,0xf8,0xf9,0xfa,0xff,0xc4,0x00,0x1f,0x01,0x00,0x03,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,
    0x07,0x08,0x09,0x0a,0x0b,0xff,0xc4,0x00,0xb5,0x11,0x00,0x02,0x01,0x02,0x04,0x04,
    0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,0x77,0x00,0x01,0x02,0x03,0x11,0x04,
    0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,
    0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,
    0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x2a,0x35,0x36,
    0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,
    0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,
    0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x92,0x93,0x94,
    0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,
    0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,
    0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
    0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xff,0xd9};

static int xioctl(int fh, int request, void *arg)
{
    int r = 0;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

Camera::Camera(const char *dev, int w, int h)
{
    dev_name = dev;
    width = w;
    height = h;
    fd = -1;
    buffers = NULL;
    pixel_format = V4L2_PIX_FMT_YUYV;
    image_buffer = new unsigned char[w * h * 3];
}

Camera::~Camera()
{
    if (buffers) {
        delete [] buffers;
    }
    if (image_buffer) {
        delete [] image_buffer;
    }
}

bool Camera::open_device()
{
    struct stat st;

    if (stat(dev_name, &st) == -1) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
        return false;
    }
    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", dev_name);
        return false;
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
        return false;
    }
    return true;
}

bool Camera::init_mmap()
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", dev_name);
        } else {
            fprintf(stderr, "VIDIOC_REQBUFS");
        }
        return false;
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        return false;
    }

    buffers = new Buffer[req.count];

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        return false;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            fprintf(stderr, "VIDIOC_QUERYBUF");
            return false;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
            mmap(NULL /* start anywhere */,
                    buf.length,
                    PROT_READ | PROT_WRITE /* required */,
                    MAP_SHARED /* recommended */,
                    fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start) {
            fprintf(stderr, "mmap");
            return false;
        }
    }
    return true;
}

bool Camera::init_device()
{
    struct v4l2_capability cap;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", dev_name);
        }
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", dev_name);
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
        return false;
    }

    /* Select video input, video standard and tune here. */

    struct v4l2_cropcap cropcap;
    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        struct v4l2_crop crop;
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    struct v4l2_format fmt;
    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = pixel_format;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
        fprintf(stderr, "VIDIOC_S_FMT");
        return false;
    }

    return true;
}

bool Camera::start()
{
    if (!open_device()) {
        return false;
    }
    if (!init_device()) {
        return false;
    }
    if (!init_mmap()) {
        return false;
    }
    for (unsigned int i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
            fprintf(stderr, "VIDIOC_QBUF");
            return false;
        }
    }
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
        fprintf(stderr, "VIDIOC_STREAMON");
        return false;
    }
    return true;
}

bool Camera::stop()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type)) {
        fprintf(stderr, "VIDIOC_STREAMOFF");
        return false;
    }
    for (unsigned int i = 0; i < n_buffers; ++i) {
        if (-1 == munmap(buffers[i].start, buffers[i].length)) {
            fprintf(stderr, "munmap");
            return false;
        }
    }
    if (-1 == close(fd)) {
        fprintf(stderr, "close");
        return false;
    }
    return true;
}

bool Camera::wait_frame()
{
    for (;;) {
        fd_set fds;
        struct timeval tv;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
                continue;
            fprintf(stderr, "select error\n");
            return false;
        }

        if (0 == r) {
            fprintf(stderr, "select timeout\n");
            return false;
        }
        return true;
    }
}

bool Camera::capture(CameraImage *ci)
{
    if (!wait_frame()) {
        return false;
    }

    struct v4l2_buffer buf;
    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
            case EAGAIN:
                // FIXME: retry in this case
                return false;

            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                fprintf(stderr, "VIDIOC_DQBUF\n");
                return false;
        }
    }

    capture_(ci, buffers[buf.index].start, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        fprintf(stderr, "VIDIOC_QBUF\n");
        return false;
    }

    return true;
}

JpegCamera::JpegCamera(const char *dev, int w, int h) : Camera(dev, w, h)
{
    pixel_format = V4L2_PIX_FMT_MJPEG;
}

JpegCamera::~JpegCamera()
{
}

void JpegCamera::capture_(CameraImage *ci, const void *ptr, size_t length)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpg_mem_src(&cinfo, jpeg_tables, sizeof(jpeg_tables));
    jpeg_read_header(&cinfo, FALSE);

    jpg_mem_src(&cinfo, ptr, length);
    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);
    cinfo.err->num_warnings = 1;

    unsigned long location = 0;
    JSAMPROW row_pointer[1];
    while (cinfo.output_scanline < cinfo.image_height) {
        row_pointer[0] = &image_buffer[location];
        jpeg_read_scanlines(&cinfo, row_pointer, 1);
        location += cinfo.image_width*cinfo.num_components;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    ci->ptr = image_buffer;
    ci->size = width * height * 3;
    ci->bits_per_pixel = 24;
}

GreyCamera::GreyCamera(const char *dev, int w, int h) : Camera(dev, w, h)
{
}

void GreyCamera::capture_(CameraImage *ci, const void *vptr, size_t length)
{
    unsigned char *ptr = (unsigned char*) vptr;
    for (size_t src = 0, dst = 0 ; src < length ; src += 2, dst++) {
        image_buffer[dst] = ptr[src];
    }
    ci->ptr = image_buffer;
    ci->size = width * height;
    ci->bits_per_pixel = 8;
}

RgbCamera::RgbCamera(const char *dev, int w, int h) : Camera(dev, w, h)
{
}

void RgbCamera::capture_(CameraImage *ci, const void *vptr, size_t length)
{
    unsigned char *ptr = (unsigned char*) vptr;
    for (size_t src = 0, dst = 0 ; src < length ; src += 2, dst += 3) {
        //TODO YUYV -> RGB
        image_buffer[dst] = ptr[src];
        image_buffer[dst+1] = ptr[src];
        image_buffer[dst+2] = ptr[src];
    }
    ci->ptr = image_buffer;
    ci->size = width * height * 3;
    ci->bits_per_pixel = 24;
}
