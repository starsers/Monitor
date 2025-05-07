#include "camera.h"
#include <sys/mman.h>



Camera::Camera() {
    // 初始化摄像头
    init_v4l2();
}
Camera::Camera(const char* device_path) : device_path(device_path) {
    // 初始化摄像头
    std::cout<<"Init camera: "<<device_path<<std::endl;
    init_v4l2();
}
Camera::Camera(const std::string& device_path) : device_path(device_path.c_str()) {
    init_v4l2();
}
Camera::~Camera() {
    // 关闭设备，取消映射
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    munmap(buffer, buf.length);
    close(fd);
}
void Camera::initFrame(cv::Mat& frame) {
    frame = cv::Mat(fmt.fmt.pix.height, fmt.fmt.pix.width, CV_8UC2, buffer);
    cv::cvtColor(frame, frame, cv::COLOR_YUV2BGR_YUYV);
}
void Camera::init_v4l2() {
    // 打开设备
    fd = open(device_path.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "无法打开摄像头设备" << std::endl;
        return;
    }

    // 设置视频格式
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        std::cerr << "无法设置视频格式" << std::endl;
        close(fd);
        return;
    }

    // 请求缓冲区
    struct v4l2_requestbuffers req;
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        std::cerr << "无法请求缓冲区" << std::endl;
        close(fd);
        return;
    }

    // 映射缓冲区
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        std::cerr << "无法查询缓冲区" << std::endl;
        close(fd);
        return;
    }
    buffer = (char*)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED) {
        std::cerr << "无法映射缓冲区" << std::endl;
        close(fd);
        return;
    }

    // 开启视频流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        std::cerr << "无法开启视频流" << std::endl;
        close(fd);
        munmap(buffer, buf.length);
        return;
    }
}
void Camera::capture_frame(cv::Mat& frame) {
    // 入队缓冲区
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        std::cerr << "无法入队缓冲区" << std::endl;
        return;
    }

    // 出队缓冲区
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        std::cerr << "无法出队缓冲区" << std::endl;
        return;
    }

    // 将数据转换为OpenCV的Mat格式
    frame = cv::Mat(fmt.fmt.pix.height, fmt.fmt.pix.width, CV_8UC2, buffer);
    cv::cvtColor(frame, frame, cv::COLOR_YUV2BGR_YUYV);
}
