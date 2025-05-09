#ifndef CAMERA_H
#define CAMERA_H

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <string>

struct BufferInfo {
    void* start;
    size_t length;
};

class Camera { 
    std::string device_path;
    int fd;
    struct v4l2_format fmt;
    struct v4l2_buffer buf;
    BufferInfo* buffers;  // 缓冲区数组
    unsigned int buffer_count;  // 缓冲区数量
    unsigned int current_buffer;  // 当前处理的缓冲区索引
    // 清理缓冲区的辅助函数
    void cleanup_buffers() {
        if (buffers) {
            for (unsigned int i = 0; i < buffer_count; ++i) {
                if (buffers[i].start != MAP_FAILED && buffers[i].start != nullptr) {
                    munmap(buffers[i].start, buffers[i].length);
                }
            }
            free(buffers);
            buffers = nullptr;
        }
    }
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
