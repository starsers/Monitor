#include "CamFactory.h"

#ifdef __linux__
#include "LinuxCam.h"
#elif defined(_WIN32)
#include "WindowCam.h"
#endif

std::unique_ptr<BaseCam> CamFactory::createCamera(const std::string& device_path) {
    std::string path = device_path.empty() ? 
        #ifdef __linux__
            "/dev/video0"
        #else
            "0"
        #endif
        : device_path;

    #ifdef __linux__
        return std::make_unique<LinuxCam>(path);
    #elif defined(_WIN32)
        return std::make_unique<WindowCam>(path);
    #else
        // 其他平台不支持，返回空指针
        return nullptr;
    #endif
}