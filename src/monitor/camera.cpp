#include "camera.h"

Camera::Camera() : device_path("/dev/video0"),
                   fd(-1),
                   buffer_count(0),
                   buffers(nullptr),
                   current_buffer(0)
{
    memset(&fmt, 0, sizeof(fmt)); // 初始化 fmt 结构体
    // 初始化摄像头
    init_v4l2();
}

Camera::Camera(const char *device_path) : device_path(device_path),
                                          fd(-1),
                                          buffer_count(0),
                                          buffers(nullptr),
                                          current_buffer(0)
{
    memset(&fmt, 0, sizeof(fmt)); // 初始化 fmt 结构体
    std::cout << "Init camera: " << device_path << std::endl;
    init_v4l2();
}

Camera::Camera(const std::string &device_path) : device_path(device_path), // 使用 std::string 而不是 c_str()
                                                 fd(-1),
                                                 buffer_count(0),
                                                 buffers(nullptr),
                                                 current_buffer(0)
{
    memset(&fmt, 0, sizeof(fmt)); // 初始化 fmt 结构体
    init_v4l2();
}
Camera::~Camera() {
    if (fd >= 0) {  // 检查fd是否有效
        // 关闭设备，取消映射
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        
        // 清理所有缓冲区
        cleanup_buffers();
        
        close(fd);
        fd = -1;  // 防止重复关闭
    }
}
void Camera::initFrame(cv::Mat& frame) {
    // 检查是否有可用的缓冲区
    if (buffer_count > 0 && buffers != nullptr && buffers[0].start != nullptr) {
        // 使用第一个缓冲区初始化帧
        frame = cv::Mat(fmt.fmt.pix.height, fmt.fmt.pix.width, CV_8UC2, buffers[0].start);
        cv::cvtColor(frame, frame, cv::COLOR_YUV2BGR_YUYV);
    } else {
        // 如果没有可用缓冲区，创建一个空帧
        frame = cv::Mat(fmt.fmt.pix.height, fmt.fmt.pix.width, CV_8UC3, cv::Scalar(0, 0, 0));
    }
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
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        std::cerr << "无法请求缓冲区" << std::endl;
        close(fd);
        return;
    }

     // 更新实际获得的缓冲区数量
     buffer_count = req.count;
    
    // 安全地分配缓冲区结构数组
    buffer_count = req.count;
    buffers = static_cast<BufferInfo*>(calloc(buffer_count, sizeof(BufferInfo)));
    if (!buffers) {
        std::cerr << "无法分配缓冲区内存" << std::endl;
        close(fd);
        return;
    }

    // 确保内存已清零
    memset(buffers, 0, buffer_count * sizeof(BufferInfo));


    // 映射所有缓冲区
    for (unsigned int i = 0; i < buffer_count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        // 查询缓冲区
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            std::cerr << "无法查询缓冲区 " << i << std::endl;
            cleanup_buffers();
            close(fd);
            return;
        }
        
        // 映射缓冲区
        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            std::cerr << "无法映射缓冲区 " << i << std::endl;
            cleanup_buffers();
            close(fd);
            return;
        }
        
        // 入队缓冲区以开始捕获
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            std::cerr << "无法入队缓冲区 " << i << std::endl;
            cleanup_buffers();
            close(fd);
            return;
        }
    }

    // 开启视频流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        std::cerr << "无法开启视频流" << std::endl;
        cleanup_buffers();  // 使用我们的清理函数而不是直接 munmap
        close(fd);
        return;
    }
}
void Camera::capture_frame(cv::Mat& frame) {
    
        // 准备出队缓冲区
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        // 出队缓冲区
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            std::cerr << "无法出队缓冲区" << std::endl;
            return;
        }
        
        // 记录当前缓冲区索引
        current_buffer = buf.index;
        
        // 将数据转换为OpenCV的Mat格式
        frame = cv::Mat(fmt.fmt.pix.height, fmt.fmt.pix.width, CV_8UC2, buffers[current_buffer].start);
        cv::cvtColor(frame, frame, cv::COLOR_YUV2BGR_YUYV);
        
        // 处理完毕，再次入队缓冲区
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            std::cerr << "无法入队缓冲区" << std::endl;
            return;
        }
}
