#ifndef CAMERA_H
#define CAMERA_H

#include "BaseCam.h"
#include <memory>

// Camera类现在是一个适配器，包装BaseCam实现
class Camera {
private:
    std::unique_ptr<BaseCam> camera;

public:
    Camera();
    Camera(const char* device_path);
    Camera(const std::string& device_path);
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    ~Camera() = default;
    
    void capture_frame(cv::Mat& frame);
    void init_v4l2(); // 实际调用camera->init()
    void initFrame(cv::Mat& frame);
};

#endif // CAMERA_H