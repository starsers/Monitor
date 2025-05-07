#include "monitor.h"
#include <sys/mman.h>




Monitor::Monitor(const char* device_path):device_path(device_path) {
    camera = new Camera(device_path);
    init();
}
Monitor::Monitor(const std::string& device_path):device_path(device_path){
    camera = new Camera(device_path);
    init();
}
// 初始化 OpenGL 纹理
void Monitor::init_texture(int width, int height) {
    if (textureID == 0) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
        std::cout << "Texture ID: " << textureID << std::endl;
    }
}
void Monitor::init(){
    // 初始化摄像头
    if(camera == nullptr){
        camera = new Camera(device_path);
    }
    textureID = 0;
    // 捕获一帧图像
    camera->capture_frame(frame);
    // 初始化 OpenGL 纹理
    init_texture(frame.cols, frame.rows);
}
void Monitor::update_texture(const cv::Mat& frame) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_BGR, GL_UNSIGNED_BYTE, frame.data);
}
void Monitor::capture(){
    if(camera == nullptr){
        camera = new Camera(device_path);
    }
    // 捕获一帧图像
    camera->capture_frame(frame);
}
void Monitor::update(){
    update_texture(frame);
}

void Monitor::display_camera_frame(const cv::Mat& frame) {
    // 更新纹理内容
    update_texture(frame);

    // 使用 ImGui 显示纹理
    ImGui::Begin("Monitor");
    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(frame.cols, frame.rows));
    ImGui::End();
}
void Monitor::display(){
    display_camera_frame(frame);
}
void Monitor::display_dynamic(){

    // 捕获一帧图像
    camera->capture_frame(frame);

    display();
}
void Monitor::destroy(){
    delete camera;
    camera = nullptr;
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}