/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   v4l2_base.h
 * Author: sobig
 *
 * Created on 2018年11月16日, 下午 5:21
 */

#include <linux/videodev2.h>

#ifndef V4L2_BASE_H
#define V4L2_BASE_H

typedef enum buf_type {
    BUF_TYPE_VIDEO_CAPTURE,
    BUF_TYPE_VIDEO_OUTPUT,
} buf_type_;

typedef enum io_memory {
    IO_MEMORY_READ,
    IO_MEMORY_MMAP,
    IO_MEMORY_USERPTR,
    IO_MEMORY_DMABUF,
} io_memory_ ;

struct v4l2_setting {
    buf_type_ type;
    io_memory_ io;
    int width;
    int height;
    unsigned int format;
    struct v4l2_crop crop;
};

struct buffer {
    int index;
    void *start;
    int length;
};

struct v4l2_buf {
    int count;
    struct v4l2_buffer vbuffer[VIDEO_MAX_FRAME];
};

struct v4l2_base {
    int fd;
    char dev_name[256];
    struct v4l2_setting v4l2setting;
    struct buffer *buffers;
    struct v4l2_buf v4l2buf;
};

struct v4l2_base* v4l2_open(char *dev_name);
void v4l2_close(struct v4l2_base *v4l2);

int v4l2_init(struct v4l2_base *v4l2, struct v4l2_setting *setting);
int v4l2_req_buf(struct v4l2_base *v4l2, int *pcount, struct buffer *buffers);

int v4l2_start_streaming(struct v4l2_base *v4l2);
int v4l2_stop_streaming(struct v4l2_base *v4l2);

int v4l2_qbuf(struct v4l2_base *v4l2, struct buffer *buff);
struct buffer* v4l2_dqbuf(struct v4l2_base *v4l2);

int v4l2_uninit(struct v4l2_base *v4l2);

#endif /* V4L2_BASE_H */
