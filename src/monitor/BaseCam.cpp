#include "BaseCam.h"

#ifdef __linux__
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <opencv2/videoio.hpp>
#endif

std::vector<std::string> BaseCam::getAvailableDevices() {
    std::vector<std::string> devices;
    
#ifdef __linux__
    // 在Linux上扫描/dev/video*设备
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name(entry->d_name);
            if (name.find("video") == 0) {
                std::string path = "/dev/" + name;
                // 检查是否可访问
                struct stat st;
                if (stat(path.c_str(), &st) == 0 && S_ISCHR(st.st_mode)) {
                    devices.push_back(path);
                }
            }
        }
        closedir(dir);
    }
#elif defined(_WIN32)
    // 在Windows上尝试打开前10个摄像头
    for (int i = 0; i < 10; ++i) {
        cv::VideoCapture temp(i);
        if (temp.isOpened()) {
            devices.push_back(std::to_string(i));
            temp.release();
        }
    }
#endif

    return devices;
}

std::string BaseCam::getDevicePath(int index) {
#ifdef __linux__
    return "/dev/video" + std::to_string(index);
#else
    return std::to_string(index);
#endif
}

std::string BaseCam::convertDevicePath(const std::string& path) {
#ifdef __linux__
    // 如果是纯数字，转换为/dev/video格式
    if (std::all_of(path.begin(), path.end(), [](char c) { return std::isdigit(c) || c == ' '; })) {
        try {
            int index = std::stoi(path);
            return "/dev/video" + std::to_string(index);
        } catch (...) {
            return "/dev/video0";
        }
    }
    // 已经是/dev/video格式则直接返回
    return path;
#elif defined(_WIN32)
    // 如果是/dev/video格式，提取数字
    if (path.find("/dev/video") == 0) {
        try {
            return path.substr(10);
        } catch (...) {
            return "0";
        }
    }
    // 已经是数字则直接返回
    return path;
#endif
}