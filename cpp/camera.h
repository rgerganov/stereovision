#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <stddef.h>

struct CameraImage
{
    const unsigned char *ptr;
    size_t size;
    int bits_per_pixel;
};

class Camera
{
    public:
        Camera(const char *dev, int w, int h);
        virtual ~Camera();
        bool start();
        bool capture(CameraImage *);
        bool stop();

    protected:
        bool open_device();
        bool init_device();
        bool init_mmap();
        bool wait_frame();

        virtual void capture_(CameraImage *dest, const void *src_ptr, size_t src_len) = 0;

        const char *dev_name;
        struct Buffer {
            void *start;
            size_t length;
        } *buffers;
        unsigned int n_buffers;
        int fd;
        int width, height;
        unsigned int pixel_format;
        unsigned char *image_buffer;
};

class JpegCamera : public Camera
{
    public:
        JpegCamera(const char *dev, int w, int h);
        virtual ~JpegCamera();

    protected:
        virtual void capture_(CameraImage *dest, const void *src_ptr, size_t src_len);
};

class GreyCamera : public Camera
{
    public:
        GreyCamera(const char *dev, int w, int h);

    protected:
        virtual void capture_(CameraImage *dest, const void *src_ptr, size_t src_len);
};

class RgbCamera : public Camera
{
    public:
        RgbCamera(const char *dev, int w, int h);

    protected:
        virtual void capture_(CameraImage *dest, const void *src_ptr, size_t src_len);
};
#endif
