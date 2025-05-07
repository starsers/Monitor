#include "monitor.h"
#include <sys/mman.h>

// 实际定义全局变量
GLuint textureID = 0;

const char* device_path = "/dev/video0";

int fd;
struct v4l2_format fmt;
struct v4l2_buffer buf;
char* buffer;

// 初始化 OpenGL 纹理
void init_texture(int width, int height) {
    if (textureID == 0) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
    }
}
void update_texture(const cv::Mat& frame) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_BGR, GL_UNSIGNED_BYTE, frame.data);
}
void init_v4l2() {
    // 打开设备
    fd = open(device_path, O_RDWR);
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

void capture_frame(cv::Mat& frame) {
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

void display_camera_frame(const cv::Mat& frame) {
    // 第一次运行时初始化纹理
    static bool initialized = false;
    if (!initialized) {
        init_texture(frame.cols, frame.rows);
        initialized = true;
    }

    // 更新纹理内容
    update_texture(frame);

    // 使用 ImGui 显示纹理
    ImGui::Begin("摄像头画面");
    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(frame.cols, frame.rows));
    ImGui::End();
}

// int main() {
//     init_v4l2();

//     cv::Mat frame;
//     while (true) {
//         capture_frame(frame);
//         display_camera_frame(frame);

//         // 这里假设已经有ImGui的主循环和渲染逻辑，此部分代码省略

//         if (cv::waitKey(1) == 27) {  // 按Esc退出
//             break;
//         }
//     }

//     // 关闭设备，取消映射
//     enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     ioctl(fd, VIDIOC_STREAMOFF, &type);
//     munmap(buffer, buf.length);
//     close(fd);

//     return 0;
// }