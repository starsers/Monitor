#ifndef MONITOR_H
#define MONITOR_H

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
// Windows 下不支持 sys/ioctl.h，需要屏蔽或替换
#if defined(__linux__)
#include <sys/ioctl.h>
#elif _WIN32
#include <windows.h>
#include <io.h> // 替代表头文件
#endif
#include <imgui.h>
#include <GL/gl.h>
#include "Camera.h"
#include <string>
#include <thread>  // 添加线程支持
#include <atomic>  // 添加原子变量支持
#include <mutex>   // 添加互斥锁支持
#include <queue>   // 添加队列支持
#include <condition_variable> // 添加条件变量支持

#include <string>
#include <chrono>
#include <vector>

struct RecordInfo {
    std::string filename;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
};

class Monitor {
    std::string device_path;
    Camera* camera;
    GLuint textureID;
    void init_texture(int width, int height);
    void update_texture(const cv::Mat& frame);
    
    void display_camera_frame(const cv::Mat& frame);// 显示摄像头图像
    void takephoto();
    void record();// 录制视频

    RecordInfo record_info_temp; // 录制信息
    std::queue<RecordInfo> record_info_queue;
    std::mutex record_info_mutex; // 保护队列并发访问

    // 异步录制所需的成员变量
    std::thread recording_thread;
    std::atomic<bool> is_recording{false};
    std::atomic<bool> stop_recording{false};
    std::string video_filename;
    int video_codec;
    double video_fps;
    
    std::mutex frame_mutex;
    std::queue<cv::Mat> frame_queue;
    std::condition_variable frame_cv;
    
    // 录制线程的工作函数
    void recording_worker();

    // 计算帧之间的时间间隔（微秒）
    int64_t frame_interval_us = static_cast<int64_t>(1000000.0 / grabbing_fps);
    
    // 用于帧率控制的时间点
    std::chrono::time_point<std::chrono::high_resolution_clock> next_frame_time;

    // 异步视频帧采集所需的成员变量
    std::thread frame_grabber_thread;
    std::atomic<bool> is_frame_grabbing{false};
    std::atomic<bool> stop_frame_grabbing{false};
    double grabbing_fps; // 期望的视频帧采集帧率
    
    // 异步视频帧采集线程的工作函数
    void frame_grabber_worker();

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
        // 停止异步视频帧采集
        stop_frame_grabbing_function();
            
        // 停止任何正在进行的录制
        stop_async_recording();
        
        if (camera != nullptr) {
            delete camera;
            camera = nullptr;
        }
        
        // 释放 OpenGL 纹理
        destroy();
    }
    void start_window();
    void show_camera();
    void end_window();
    void capture_button();
    void record_button();

    // 异步录制方法
    void start_async_recording(const std::string& filename = "output.mp4", 
                                int codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1'), 
                                double fps = 30.0);
    void stop_async_recording();
    bool is_recording_active() const;

    // 异步视频帧采集方法
    void start_frame_grabbing_function(double fps = 30.0);
    void stop_frame_grabbing_function();
    bool is_frame_grabbing_active() const;
    bool get_latest_recorded_frame(cv::Mat& out_frame);
    
    std::vector<RecordInfo> get_all_record_info();

    void listAvailableCameras();
};


#endif // MONITOR_H
