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
#include "camera.h"
#include <string>






class Monitor {
    std::string device_path;
    Camera* camera;
    GLuint textureID;
    void init_texture(int width, int height);
    void update_texture(const cv::Mat& frame);
    
    void display_camera_frame(const cv::Mat& frame);// 显示摄像头图像
public:
    cv::Mat frame;
    Monitor(const char* device_path = "/dev/video0");
    Monitor(const std::string& device_path);
    void init();
    void capture();// 捕获一帧图像
    void update();// 更新纹理
    void destroy();// 释放资源，后需init
    void display();// 显示摄像头图像
    void display_dynamic();
    ~Monitor() {
        if (camera != nullptr) {
            delete camera;
            camera = nullptr;
        }
        // 释放 OpenGL 纹理
        destroy();
    }
};


#endif // MONITOR_H
