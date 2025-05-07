#ifndef CAMERA_H
#define CAMERA_H

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <string>

class Camera { 
    std::string device_path;
    int fd;
    struct v4l2_format fmt;
    struct v4l2_buffer buf;
    char* buffer;
    public:
        Camera();
        Camera(const char* device_path);
        Camera(const std::string& device_path);
        Camera(const Camera&) = delete; // 禁止拷贝构造函数
        Camera& operator=(const Camera&) = delete; // 禁止赋值操作符重载
        ~Camera();
        void capture_frame(cv::Mat& frame);
        void init_v4l2();
        void initFrame(cv::Mat& frame);
};


#endif // CAMERA_H
