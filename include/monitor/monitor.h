#ifndef MONITOR_H
#define MONITOR_H

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <imgui.h>
#include <GL/gl.h>

// monitor.h
extern GLuint textureID;

// V4L2设备路径
extern const char* device_path;

// V4L2相关变量
extern int fd;
extern struct v4l2_format fmt;
extern struct v4l2_buffer buf;
extern char* buffer;


void init_v4l2();
void capture_frame(cv::Mat& frame) ;
void display_camera_frame(const cv::Mat& frame) ;


#endif // MONITOR_H
