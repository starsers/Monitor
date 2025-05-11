#ifndef LINUXCAM_H
#define LINUXCAM_H

#include "BaseCam.h"

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <poll.h>

struct BufferInfo {
    void* start;
    size_t length;
};

class LinuxCam : public BaseCam {
private:
    int fd = -1;
    struct v4l2_format fmt;
    struct v4l2_buffer buf;
    unsigned int buffer_count = 0;
    BufferInfo* buffers = nullptr;
    unsigned int current_buffer = 0;
    
    // 清理缓冲区的辅助函数
    void cleanup_buffers();
    void init_v4l2();

public:
    LinuxCam();
    explicit LinuxCam(const std::string& device_path);
    LinuxCam(const LinuxCam&) = delete; // 禁止拷贝
    LinuxCam& operator=(const LinuxCam&) = delete;
    ~LinuxCam() override;
    
    bool init() override;
    void release() override;
    bool capture_frame(cv::Mat& frame) override;
    bool is_opened() const override { return fd >= 0; }
    void initFrame(cv::Mat& frame) override;
};
#endif // __linux__

#endif // LINUXCAM_H