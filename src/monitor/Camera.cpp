#include "Camera.h"
#include "CamFactory.h"

Camera::Camera() {
    camera = CamFactory::createCamera();
    camera->init();
}

Camera::Camera(const char* device_path) {
    camera = CamFactory::createCamera(device_path);
    camera->init();
}

Camera::Camera(const std::string& device_path) {
    camera = CamFactory::createCamera(device_path);
    camera->init();
}

void Camera::capture_frame(cv::Mat& frame) {
    camera->capture_frame(frame);
}

void Camera::init_v4l2() {
    camera->init();
}

void Camera::initFrame(cv::Mat& frame) {
    camera->initFrame(frame);
}