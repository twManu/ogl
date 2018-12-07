/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   v4l2_base.cc
 * Author: sobig
 * 
 * Created on 2018年11月16日, 下午 5:21
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "v4l2_base.h"

/*  Dynamic debug. */
static int v4l2basedbg = 1;

#define V4L2_MSG(fmt, ...) \
        do { if (v4l2basedbg) fprintf(stderr, "MSG[V4L2BASE]:%s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); } while (0)
#define V4L2_ERR(fmt, ...) \
        do { fprintf(stderr, "ERR[V4L2BASE]:%s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); } while (0)

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
        if (r < 0)
            V4L2_ERR("request: %d", request);
    } while (-1 == r && EINTR == errno);

    return r;
}

static void process_image(const void *p, int size)
{
    FILE *fp=fopen("frame.raw","wb");
    fwrite(p, size, 1, fp);
    fflush(fp);
    fclose(fp);
}

struct v4l2_base* v4l2_open(char *dev_name)
{
    struct stat st;
    struct v4l2_base *v4l2 = NULL; 

    if (-1 == stat(dev_name, &st)) {
       V4L2_ERR("Cannot identify '%s': %d, %s",
                dev_name, errno, strerror(errno));
        return NULL;
    }

    if (!S_ISCHR(st.st_mode)) {
        V4L2_ERR("%s is no device", dev_name);
        return NULL;
    }

    v4l2 = (struct v4l2_base *)calloc(1, sizeof(*v4l2));
    if (!v4l2) {
        V4L2_ERR("Out of memory: %d, %s", errno, strerror(errno));
        return NULL;
    }
    
    v4l2->fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (v4l2->fd < 0) {
        V4L2_ERR("Cannot open '%s': %d, %s",
                dev_name, errno, strerror(errno));
        return NULL;    
    }
    
    strncpy(v4l2->dev_name, dev_name, strlen(dev_name));
    V4L2_MSG("open (%s) successfully", v4l2->dev_name);
    return v4l2;
}

void v4l2_close(struct v4l2_base *v4l2)
{
    if (v4l2->fd > 0) {
        V4L2_MSG("close: %s successfully", v4l2->dev_name);
        close(v4l2->fd);
    }
    free(v4l2);
}

int v4l2_init(struct v4l2_base *v4l2, struct v4l2_setting *setting)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;

    CLEAR(cap);
    if (-1 == xioctl(v4l2->fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            V4L2_ERR("%s is no V4L2 device", v4l2->dev_name);
        } else {
            V4L2_ERR("VIDIOC_QUERYCAP: %d, %s", errno, strerror(errno));
        }
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        V4L2_ERR("%s is no video capture device", v4l2->dev_name);
        return -1;
    }

    switch (setting->io) {
        case IO_MEMORY_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                V4L2_ERR("%s does not support read i/o", v4l2->dev_name);
                return -1;
            }
            break;

        case IO_MEMORY_MMAP:
        case IO_MEMORY_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                V4L2_ERR("%s does not support streaming i/o", v4l2->dev_name);
                return -1;
            }
            break;
            
        default:
            break;
            
    }
            
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = setting->width;
    fmt.fmt.pix.height = setting->height;
    fmt.fmt.pix.pixelformat = setting->format;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    
    V4L2_MSG("res: %dx%d, format: %u, field: %u",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fmt.fmt.pix.pixelformat,
            fmt.fmt.pix.field);
    
    if (-1 == xioctl(v4l2->fd, VIDIOC_S_FMT, &fmt)) {
        V4L2_ERR("VIDIOC_S_FMT: %d, %s", errno, strerror(errno));
        return -1;
    }
    
    v4l2->v4l2setting.io = setting->io;
    v4l2->v4l2setting.type = setting->type;    
    v4l2->v4l2setting.width = setting->width;
    v4l2->v4l2setting.height = setting->height;
    v4l2->v4l2setting.format = setting->format;

    return 0;
}

int v4l2_req_buf(struct v4l2_base *v4l2, int *pcount, struct buffer *buffers)
{
    struct v4l2_requestbuffers req;
    
    CLEAR(req);
    req.count  = *pcount;
    switch (v4l2->v4l2setting.io) {
        case IO_MEMORY_READ:
            break;

        case IO_MEMORY_MMAP:
            break;
            
        case IO_MEMORY_USERPTR:
            req.memory = V4L2_MEMORY_USERPTR;
            req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            
            break;
            
        default:
            break;
    }
    
    if (-1 == xioctl(v4l2->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            V4L2_ERR("%s does not support user pointer i/o", v4l2->dev_name);            
        } else {
            V4L2_ERR("VIDIOC_REQBUFS: %d, %s", errno, strerror(errno));
        }
        return -1;
    }
    
    *pcount = req.count;
    v4l2->buffers = buffers;
    v4l2->v4l2buf.count = req.count; 
    
    return 0;
}

int v4l2_start_streaming(struct v4l2_base *v4l2)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (v4l2->v4l2setting.io) {
        case IO_MEMORY_READ:
            /* Nothing to do. */
            break;

        case IO_MEMORY_MMAP:
            break;

        case IO_MEMORY_USERPTR:
            for (i = 0; i < v4l2->v4l2buf.count; ++i) {
                struct v4l2_buffer buf;
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;
                buf.index = v4l2->buffers[i].index;
                buf.m.userptr = (unsigned long)v4l2->buffers[i].start;
                buf.length = v4l2->buffers[i].length;
                
                V4L2_MSG("type:%d, memory:%d, index:%d, buf:%p, len: %d",
                        buf.type,
                        buf.memory,
                        buf.index,
                        (void *)buf.m.userptr,
                        buf.length);
                
                if (-1 == xioctl(v4l2->fd, VIDIOC_QBUF, &buf)) {
                    V4L2_ERR("VIDIOC_QBUF: %d, %s", errno, strerror(errno));
                    return -1;
                }
            }
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (-1 == xioctl(v4l2->fd, VIDIOC_STREAMON, &type)) {
               V4L2_ERR("VIDIOC_STREAMON: %d, %s", errno, strerror(errno));
               return -1;
            }
            break;
            
        default:
            break;
    }
    return 0;
}

int v4l2_stop_streaming(struct v4l2_base *v4l2)
{
    enum v4l2_buf_type type;

    switch (v4l2->v4l2setting.io) {
        case IO_MEMORY_READ:
            /* Nothing to do. */
            break;

        case IO_MEMORY_MMAP:
            break;
            
        case IO_MEMORY_USERPTR:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            
            if (-1 == xioctl(v4l2->fd, VIDIOC_STREAMOFF, &type)) {
                V4L2_ERR("VIDIOC_STREAMOFF: %d, %s", errno, strerror(errno));
                return -1;
            }
            break;
            
        default:
            break;
    }
    return 0;
}


int v4l2_qbuf(struct v4l2_base *v4l2, struct buffer *buff)
{
    struct v4l2_buffer buf;
    CLEAR(buf);
    
    switch (v4l2->v4l2setting.io) {
        case IO_MEMORY_READ:
            /* Nothing to do. */
            break;

        case IO_MEMORY_MMAP:
            break;
            
        case IO_MEMORY_USERPTR:
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = buff->index;
            buf.m.userptr = (unsigned long)buff->start;
            buf.length = buff->length;
            
            if (-1 == xioctl(v4l2->fd, VIDIOC_QBUF, &buf)) {
                V4L2_ERR("VIDIOC_DQBUF: %d, %s", errno, strerror(errno));
                return -1;
            }
            break;
            
        default:
            break;
    }
    
    return 0;
}

struct buffer* v4l2_dqbuf(struct v4l2_base *v4l2)
{
    struct v4l2_buffer buf;
    fd_set fds;
    struct timeval tv;
    int r;

    
    FD_ZERO(&fds);
    FD_SET(v4l2->fd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select(v4l2->fd + 1, &fds, NULL, NULL, &tv);

    if (-1 == r) {
        if (EINTR == errno)
            return NULL;
    }

    if (0 == r) {
        fprintf(stderr, "select timeout\n");
        return NULL;
    }
    
    switch (v4l2->v4l2setting.io) {
        case IO_MEMORY_READ:
            /* Nothing to do. */
            break;

        case IO_MEMORY_MMAP:
            break;
            
        case IO_MEMORY_USERPTR:
            CLEAR(buf);
            
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            
            if (-1 == xioctl(v4l2->fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                    case EAGAIN:
                        V4L2_ERR("VIDIOC_DQBUF: %d, %s", errno, strerror(errno));
                        return NULL;

                    case EIO:
                        V4L2_ERR();
                            /* Could ignore EIO, see spec. */

                            /* fall through */

                    default:
                        V4L2_ERR("VIDIOC_DQBUF: %d, %s", errno, strerror(errno));
                        return NULL;                        
                }
            }
            break;
            
        default:
            break;
    }
    
    //process_image((void *)buf.m.userptr, buf.bytesused);
    return &v4l2->buffers[buf.index];

}

int v4l2_uninit(struct v4l2_base *v4l2)
{
    int nbuf = 0;
    
    if (v4l2_req_buf(v4l2, &nbuf, NULL) < 0)
        return -1;
    return 0;
}