#ifndef WINDOWSCAM_H
#define WINDOWSCAM_H

#include "BaseCam.h"

#ifdef _WIN32
#include <opencv2/videoio.hpp>

class WindowsCam : public BaseCam {
private:
    cv::VideoCapture cap;
    cv::Size frame_size{640, 480}; // 默认尺寸

public:
    WindowsCam();
    explicit WindowsCam(const std::string& device_path);
    WindowsCam(const WindowsCam&) = delete;
    WindowsCam& operator=(const WindowsCam&) = delete;
    ~WindowsCam() override;
    
    bool init() override;
    void release() override;
    bool capture_frame(cv::Mat& frame) override;
    bool is_opened() const override { return cap.isOpened(); }
    void initFrame(cv::Mat& frame) override;
};
#endif // _WIN32

#endif // WINDOWSCAM_H