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
#include <thread>  // 添加线程支持
#include <atomic>  // 添加原子变量支持
#include <mutex>   // 添加互斥锁支持
#include <queue>   // 添加队列支持
#include <condition_variable> // 添加条件变量支持

#include <string>
#include <chrono>
#include <vector>

#include <future>

#include "database.h"

struct RecordInfo {
    std::string filename;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
};

class Monitor {
    SQLiteDatabase* db;
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

        // 在 private 或 public 区域添加以下成员变量
    bool schedule_recording = false;           // 是否已设置定时录制
    bool schedule_stop_recording = false;      // 是否已设置定时停止录制
    std::chrono::system_clock::time_point scheduled_record_time;    // 计划开始录制的时间
    std::chrono::system_clock::time_point scheduled_stop_time;      // 计划停止录制的时间
    
    // 定时录制相关成员函数
    void schedule_record_button();             // 定时开始录制按钮
    void schedule_stop_button();               // 定时结束录制按钮
    bool check_and_execute_scheduled_tasks();  // 检查并执行定时任务
    
    // 视频查找相关变量和函数
    void video_search_ui();                    // 视频查找界面
    std::string search_start_time_str = "";    // 开始时间字符串
    std::string search_end_time_str = "";      // 结束时间字符串
    std::string search_target_time_str = "";   // 目标时间字符串
    std::vector<std::string> search_results;   // 存储搜索结果
    bool show_search_results = false;          // 是否显示搜索结果
public:
    // 将chrono::time_point转换为Unix 时间戳
    static std::string convert_time_to_unix_timestamp(const std::chrono::system_clock::time_point& time_point);
public:
    std::string saving_path="./";
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

    void setDatabase(SQLiteDatabase db) {
        delete this->db;
        this->db = new SQLiteDatabase(db);
        this->db->connect();
        init_database();
    }
    SQLiteDatabase getDatabase() {
        return *this->db;
    }
    // 数据库相关函数
    void init_database();
    std::vector<std::vector<std::string>> search_video_from_timemap(std::string start_time, std::string end_time);
    std::vector<std::string> search_video_from_target_time(std::string target_time);
    std::vector<RecordInfo> get_recent_record_info(int num_records=10);
    bool insert_record_info(const RecordInfo& record_info);
    bool delete_record_info(const std::string& filename);

    
};



#endif // MONITOR_H
