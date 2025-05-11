#ifndef BASECAM_H
#define BASECAM_H

#include <opencv2/opencv.hpp>
#include <string>
#include <mutex>
#include <vector>

class BaseCam {
protected:
    std::mutex cam_mutex;
    std::string device_path;
    bool initialized = false;

public:
    BaseCam() = default;
    explicit BaseCam(const std::string& device_path) : device_path(device_path) {}
    virtual ~BaseCam() = default;
    
    // 核心功能接口
    virtual bool init() = 0;  // 初始化摄像头
    virtual void release() = 0;  // 释放资源
    virtual bool capture_frame(cv::Mat& frame) = 0;  // 捕获一帧
    virtual bool is_opened() const = 0;  // 检查是否已打开
    virtual void initFrame(cv::Mat& frame) = 0;  // 初始化首帧
    
    // 获取/设置设备路径
    std::string getDevicePath() const { return device_path; }
    void setDevicePath(const std::string& path) { device_path = path; }
    
    // 静态帮助方法
    static std::vector<std::string> getAvailableDevices();
    static std::string getDevicePath(int index);
    static std::string convertDevicePath(const std::string& path);
};

#endif // BASECAM_H