#include "WindowCam.h"

#ifdef _WIN32
#include <algorithm>

WindowCam::WindowCam() : BaseCam("0"), initialized(false) {
    init();
}

WindowCam::WindowCam(const std::string& device_path) : BaseCam(device_path), initialized(false) {
    std::cout << "Init camera: " << device_path << std::endl;
    init();
}

WindowCam::~WindowCam() {
    release();
}

bool WindowCam::init() {
    std::lock_guard<std::mutex> lock(cam_mutex);
    
    // 解析设备路径
    int device_id = 0;
    
    // 从Linux风格路径中提取ID
    if (device_path.find("/dev/video") == 0) {
        try {
            device_id = std::stoi(device_path.substr(10));
        } catch (...) {
            device_id = 0;
        }
    }
    // 纯数字路径
    else if (std::all_of(device_path.begin(), device_path.end(), [](char c) { 
             return std::isdigit(c) || c == ' '; })) {
        try {
            device_id = std::stoi(device_path);
        } catch (...) {
            device_id = 0;
        }
    }
    
    // 尝试打开摄像头
    cap.open(device_id);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头设备: " << device_id << std::endl;
        return false;
    }
    
    // 设置摄像头属性
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    
    // 获取实际尺寸
    frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    
    // 准备好第一帧
    cv::Mat temp;
    if (cap.read(temp)) {
        current_frame = temp.clone();
    } else {
        current_frame = cv::Mat(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    }
    
    initialized = true;
    return true;
}

void WindowCam::release() {
    std::lock_guard<std::mutex> lock(cam_mutex);
    
    if (cap.isOpened()) {
        cap.release();
    }
    
    initialized = false;
}

bool WindowCam::capture_frame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(cam_mutex);
    
    if (!cap.isOpened()) {
        std::cerr << "摄像头未打开，无法捕获帧" << std::endl;
        return false;
    }
    
    bool success = cap.read(current_frame);
    if (!success || current_frame.empty()) {
        std::cerr << "无法读取摄像头帧" << std::endl;
        return false;
    }
    
    frame = current_frame.clone();
    return true;
}

void WindowCam::initFrame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(cam_mutex);
    
    if (!current_frame.empty()) {
        frame = current_frame.clone();
    } else if (cap.isOpened()) {
        // 尝试读取一帧
        cv::Mat temp;
        if (cap.read(temp)) {
            current_frame = temp.clone();
            frame = current_frame.clone();
        } else {
            frame = cv::Mat(frame_height, frame_width, CV_8UC3, cv::Scalar(0, 0, 0));
        }
    } else {
        frame = cv::Mat(frame_height, frame_width, CV_8UC3, cv::Scalar(0, 0, 0));
    }
}
#endif // _WIN32