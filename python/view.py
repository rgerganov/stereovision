import Tkinter
import Image, ImageTk
import os
import time
import mmap
import select
import Image
import fcntl
import sys
from StringIO import StringIO
from v4l2 import *

class ImageBuffer:

    huffman_table = \
            '\xFF\xC4\x01\xA2\x00\x00\x01\x05\x01\x01\x01\x01'\
            '\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02'\
            '\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x01\x00\x03'\
            '\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00'\
            '\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09'\
            '\x0A\x0B\x10\x00\x02\x01\x03\x03\x02\x04\x03\x05'\
            '\x05\x04\x04\x00\x00\x01\x7D\x01\x02\x03\x00\x04'\
            '\x11\x05\x12\x21\x31\x41\x06\x13\x51\x61\x07\x22'\
            '\x71\x14\x32\x81\x91\xA1\x08\x23\x42\xB1\xC1\x15'\
            '\x52\xD1\xF0\x24\x33\x62\x72\x82\x09\x0A\x16\x17'\
            '\x18\x19\x1A\x25\x26\x27\x28\x29\x2A\x34\x35\x36'\
            '\x37\x38\x39\x3A\x43\x44\x45\x46\x47\x48\x49\x4A'\
            '\x53\x54\x55\x56\x57\x58\x59\x5A\x63\x64\x65\x66'\
            '\x67\x68\x69\x6A\x73\x74\x75\x76\x77\x78\x79\x7A'\
            '\x83\x84\x85\x86\x87\x88\x89\x8A\x92\x93\x94\x95'\
            '\x96\x97\x98\x99\x9A\xA2\xA3\xA4\xA5\xA6\xA7\xA8'\
            '\xA9\xAA\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xC2'\
            '\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xD2\xD3\xD4\xD5'\
            '\xD6\xD7\xD8\xD9\xDA\xE1\xE2\xE3\xE4\xE5\xE6\xE7'\
            '\xE8\xE9\xEA\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9'\
            '\xFA\x11\x00\x02\x01\x02\x04\x04\x03\x04\x07\x05'\
            '\x04\x04\x00\x01\x02\x77\x00\x01\x02\x03\x11\x04'\
            '\x05\x21\x31\x06\x12\x41\x51\x07\x61\x71\x13\x22'\
            '\x32\x81\x08\x14\x42\x91\xA1\xB1\xC1\x09\x23\x33'\
            '\x52\xF0\x15\x62\x72\xD1\x0A\x16\x24\x34\xE1\x25'\
            '\xF1\x17\x18\x19\x1A\x26\x27\x28\x29\x2A\x35\x36'\
            '\x37\x38\x39\x3A\x43\x44\x45\x46\x47\x48\x49\x4A'\
            '\x53\x54\x55\x56\x57\x58\x59\x5A\x63\x64\x65\x66'\
            '\x67\x68\x69\x6A\x73\x74\x75\x76\x77\x78\x79\x7A'\
            '\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x92\x93\x94'\
            '\x95\x96\x97\x98\x99\x9A\xA2\xA3\xA4\xA5\xA6\xA7'\
            '\xA8\xA9\xAA\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA'\
            '\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xD2\xD3\xD4'\
            '\xD5\xD6\xD7\xD8\xD9\xDA\xE2\xE3\xE4\xE5\xE6\xE7'\
            '\xE8\xE9\xEA\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA'

    def __init__(self):
        self.length = 0
        self.start = 0

    def read(self, bytesused):
        buffer = self.start.read(bytesused)
        self.start.seek(0)
        instream = StringIO(buffer)
        outstream = StringIO()

        hdr = instream.read(2)
        if hdr != '\xff\xd8':
            raise Exception("JPEG image expected")
        outstream.write(hdr)
        
        # for some reason the Huffman table may be stripped
        # check if this is the case and append if needed
        has_dht = False
        while not has_dht:
            # first two bytes are magic, next two are size
            hdr = instream.read(4)
            if hdr[0] != '\xff':
                raise Exception("Unexpected marker: " + header)
            if hdr[1] == '\xc4':
                has_dht = True
            elif hdr[1] == '\xda':
                break

            # skip to the next marker.
            size = (ord(hdr[2]) << 8) + ord(hdr[3])
            outstream.write(hdr)
            outstream.write(instream.read(size-2))

        if not has_dht:
            outstream.write(self.huffman_table)
            outstream.write(hdr)

        # process the remaining data in one go
        outstream.write(instream.read())
        instream.close()
        outstream.flush()
        outstream.seek(0)
        return outstream

class Camera:
    def __init__(self, dev_name):
        self.vd = os.open(dev_name, os.O_RDWR | os.O_NONBLOCK, 0)
        self.buffers = []

    def prepare(self, width, height, format = V4L2_PIX_FMT_MJPEG):
        cp = v4l2_capability()
        fcntl.ioctl(self.vd, VIDIOC_QUERYCAP, cp)

        fmt = v4l2_format()
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
        fmt.fmt.pix.width = width
        fmt.fmt.pix.height = height
        fmt.fmt.pix.pixelformat = format
        fmt.fmt.pix.field = V4L2_FIELD_NONE
        fcntl.ioctl(self.vd, VIDIOC_S_FMT, fmt)

        req = v4l2_requestbuffers()
        req.count = 4
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
        req.memory = V4L2_MEMORY_MMAP
        fcntl.ioctl(self.vd, VIDIOC_REQBUFS, req)
        for ind in range(req.count):
            buf = v4l2_buffer()
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
            buf.memory = V4L2_MEMORY_MMAP
            buf.index = ind
            fcntl.ioctl(self.vd, VIDIOC_QUERYBUF, buf)
            buffer = ImageBuffer()
            buffer.length = buf.length
            buffer.start = mmap.mmap(self.vd, buf.length, mmap.MAP_SHARED, 
                mmap.PROT_READ | mmap.PROT_WRITE, offset=buf.m.offset)
            self.buffers.append(buffer)
        for ind in range(req.count):
            buf = v4l2_buffer()
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
            buf.memory = V4L2_MEMORY_MMAP
            buf.index = ind
            fcntl.ioctl(self.vd, VIDIOC_QBUF, buf)
        type = v4l2_buf_type(V4L2_BUF_TYPE_VIDEO_CAPTURE)
        fcntl.ioctl(self.vd, VIDIOC_STREAMON, type)

    def capture(self):
        buf = v4l2_buffer()
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
        buf.memory = V4L2_MEMORY_MMAP
        fcntl.ioctl(self.vd, VIDIOC_DQBUF, buf)
        data = self.buffers[buf.index].read(buf.bytesused)
        fcntl.ioctl(self.vd, VIDIOC_QBUF, buf)
        return data

class StereoViewer:
    def __init__(self, left_cam, right_cam, sleep_time):
        self.sleep_time = sleep_time
        self.left_cam = left_cam
        self.right_cam = right_cam

        self.root = Tkinter.Tk()
        self.root.title("Stereo vision")
        self.root.bind("<Key>", self.key_pressed)
        self.left_label = Tkinter.Label(self.root)
        self.left_label.grid(row=0, column=0)
        self.right_label = Tkinter.Label(self.root)
        self.right_label.grid(row=0, column=1)
        self.capture = Tkinter.Button(self.root, text="Capture", command=self.button_pressed)
        self.capture.grid(row=1, columnspan=2)
        self.root.after(self.sleep_time, self.refresh)
        self.root.mainloop()

    def button_pressed(self):
        ts = int(time.time() * 1000)
        self.left_img.save("left-%d.jpg" % ts)
        self.right_img.save("right-%d.jpg" % ts)
        print "Images with timestamp %d saved" % ts

    def key_pressed(self, event):
        if event.char == 'q' or event.char == 'Q':
            sys.exit(0)

    def refresh(self):
        select.select([self.left_cam.vd], [], [])
        data = self.left_cam.capture()
        self.left_img = Image.open(data)
        self.left_pimg = ImageTk.PhotoImage(self.left_img)
        self.left_label.configure(image = self.left_pimg)

        select.select([self.right_cam.vd], [], [])
        data = self.right_cam.capture()
        self.right_img = Image.open(data)
        self.right_pimg = ImageTk.PhotoImage(self.right_img)
        self.right_label.configure(image = self.right_pimg)

        self.root.after(self.sleep_time, self.refresh)

left_cam = Camera("/dev/video0")
left_cam.prepare(640, 480)
right_cam = Camera("/dev/video1")
right_cam.prepare(640, 480)
gui = StereoViewer(left_cam, right_cam, 5)
