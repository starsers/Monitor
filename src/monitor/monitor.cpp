#include "monitor.h"
#include <sys/mman.h>

Monitor::Monitor(const char* device_path):device_path(device_path) {
    camera = new Camera(device_path);
    // 把device_path中的/转换为下划线
    std::string device_path_str(device_path);
    std::replace(device_path_str.begin(), device_path_str.end(), '/', '_');
    std::string db_path = std::string(device_path_str) + ".db";
    
    db = new SQLiteDatabase(db_path);
    db->connect();
    init_database();

    init();
}
Monitor::Monitor(const std::string& device_path):device_path(device_path){
    camera = new Camera(device_path);
    // 把device_path中的/转换为下划线
    std::string device_path_str(device_path);
    std::replace(device_path_str.begin(), device_path_str.end(), '/', '_');
    std::string db_path = std::string(device_path_str) + ".db";
    db = new SQLiteDatabase(db_path);
    db->connect();
    init_database();
    init();
}
void Monitor::init_database(){
    if (!db->connect()) {
        std::cerr << "连接数据库失败" << std::endl;
        // 修复 void 函数中的返回值
        return;
    }
    
    // 修复原始字符串字面量语法
    if (!db->createTable("record_info", 
        R"(
            id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 主键，唯一标识每条记录
            video_path TEXT NOT NULL,              -- 视频文件的存储路径
            start_time INTEGER NOT NULL,           -- 开始时间（Unix 时间戳，单位为秒）
            end_time INTEGER,                      -- 结束时间（Unix 时间戳，单位为秒）
            duration INTEGER GENERATED ALWAYS AS (end_time - start_time) STORED,  -- 持续时间（可选，自动计算）
            CHECK(end_time IS NULL OR end_time >= start_time)
        )")) {
        std::cerr << "创建表失败" << std::endl;
        // 修复 void 函数中的返回值
        return;
    }
    
    // 修复缺失的对象调用
    db->execute("CREATE INDEX IF NOT EXISTS idx_start_end_time ON record_info(start_time, end_time);");
}

// 初始化 OpenGL 纹理
void Monitor::init_texture(int width, int height) {
    if (textureID == 0) {
        if (width <= 0 || height <= 0) {
            std::cerr << "无效的纹理尺寸: " << width << "x" << height << std::endl;
            return;
        }
        
        glGenTextures(1, &textureID);
        std::cout << "Texture ID: " << textureID << std::endl;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // 初始化纹理数据
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL 错误: " << err << std::endl;
        }
    }
}
void Monitor::init() {
    // 先捕获一帧以确保frame有效
    if (camera == nullptr) {
        camera = new Camera(device_path);
    }
    
    // 等待摄像头准备就绪
    cv::Mat tempFrame;
    camera->initFrame(tempFrame);
    
    if (!tempFrame.empty()) {
        frame = tempFrame;
        // 初始化 OpenGL 纹理
        init_texture(frame.cols, frame.rows);
        if (textureID != 0) {
            // 更新纹理数据
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame.cols, frame.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, frame.data);
        } else {
            std::cerr << "无法创建 OpenGL 纹理" << std::endl;
        }
        std::cout << "摄像头初始化成功，纹理 ID: " << textureID << std::endl;
    } else {
        std::cerr << "无法初始化摄像头帧，纹理未创建" << std::endl;
    }
}
void Monitor::update_texture(const cv::Mat& frame) {
    if (frame.empty()) {
        std::cerr << "update_texture: frame is empty!" << std::endl;
        return;
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_BGR, GL_UNSIGNED_BYTE, frame.data);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL 错误: " << err << std::endl;
    }
}
void Monitor::capture(){
    if(camera == nullptr){
        camera = new Camera(device_path);
    }
    // 捕获一帧图像
    camera->capture_frame(frame);
    
    // 如果正在录制，将帧添加到队列
    if (is_recording) {
        std::unique_lock<std::mutex> lock(frame_mutex);
        
        // 限制队列大小，防止内存溢出（可选）
        if (frame_queue.size() > 30) {  // 最多缓存30帧
            frame_queue.pop();
        }
        
        frame_queue.push(frame.clone());  // 使用clone以避免引用问题
        lock.unlock();
        
        // 通知录制线程有新帧可用
        frame_cv.notify_one();
    }
}
void Monitor::update(){
    update_texture(frame);
}

void Monitor::display_camera_frame(const cv::Mat& frame) {
    // 更新纹理内容
    update_texture(frame);

    // 使用 ImGui 显示纹理
    start_window();
    show_camera();
    ImGui::NextColumn();
    record_button();
    capture_button();
    end_window();
}

void Monitor::display(){
    std::lock_guard<std::mutex> lock(frame_mutex);
    if (!frame.empty()) {
        display_camera_frame(frame);
    }
}
void Monitor::display_dynamic(){
    if (is_recording) {
        // 录制时，从队列取最新帧
        cv::Mat latest;
        if (get_latest_recorded_frame(latest)) {
            frame = latest;
        }
        // 如果队列为空，可以选择不刷新或显示上一帧
    } else {
        // 非录制时，直接采集新帧
        camera->capture_frame(frame);
    }
    display();
}
void Monitor::destroy(){
    if (camera != nullptr) {
        delete camera;
        camera = nullptr;
    }
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}

// 开始异步录制
void Monitor::start_async_recording(const std::string& filename, int codec, double fps) {
    // 如果已经在录制，先停止
    if (is_recording) {
        stop_async_recording();
    }
    
    video_filename = filename;
    video_codec = codec;
    video_fps = fps;
    
    // 重置停止标志
    stop_recording = false;
    is_recording = true;
        
    // 启动录制线程
    recording_thread = std::thread(&Monitor::recording_worker, this);
    
    // 如果还没有启动异步视频帧采集，启动它
    if (!is_frame_grabbing_active()) {
        start_frame_grabbing_function(fps);
    }
}

// 停止异步录制
void Monitor::stop_async_recording() {
    if (is_recording) {
        // 设置停止标志
        stop_recording = true;
        
        // 通知可能在等待的录制线程
        frame_cv.notify_one();
        
        // 等待线程结束
        if (recording_thread.joinable()) {
            recording_thread.join();
        }
        
        is_recording = false;
    }
}

// 检查是否正在录制
bool Monitor::is_recording_active() const {
    return is_recording;
}

// 录制线程的工作函数
void Monitor::recording_worker() {
    cv::VideoWriter writer;
    bool writer_initialized = false;

    while (!stop_recording) {
        // 等待新帧或停止信号
        std::unique_lock<std::mutex> lock(frame_mutex);
        if (frame_queue.empty()) {
            // 使用条件变量等待，超时1秒以检查停止标志
            frame_cv.wait_for(lock, std::chrono::seconds(1), 
                             [this]() { return !frame_queue.empty() || stop_recording; });
            
            // 如果队列仍然为空且收到停止信号，结束循环
            if (frame_queue.empty() && stop_recording) {
                break;
            }
            
            // 如果只是超时，继续循环
            if (frame_queue.empty()) {
                continue;
            }
        }
        
        // 获取队列中的帧
        cv::Mat current_frame = frame_queue.front().clone();  // 使用clone避免引用问题
        frame_queue.pop();
        lock.unlock();
        
        // 初始化VideoWriter（在第一帧可用时）
        if (!writer_initialized) {
            writer.open(video_filename, video_codec, video_fps, 
                       cv::Size(current_frame.cols, current_frame.rows));
            
            if (!writer.isOpened()) {
                std::cerr << "无法创建视频文件：" << video_filename << std::endl;
                is_recording = false;
                return;
            }
            
            writer_initialized = true;
            std::cout << "开始录制视频到：" << video_filename << std::endl;
            record_info_temp.filename = video_filename;
            record_info_temp.start_time = std::chrono::system_clock::now();

        }
        
        // 写入帧
        writer.write(current_frame);
    }
    
    // 释放VideoWriter
    if (writer_initialized) {
        writer.release();
        std::cout << "视频录制完成：" << video_filename << std::endl;
        record_info_temp.end_time = std::chrono::system_clock::now();
        record_info_queue.push(record_info_temp);
        std::cout << "录制信息已保存到队列" << std::endl;
    }
}

void Monitor::start_window() {
    // 创建窗口
    ImGui::Begin("Monitor");
}

void Monitor::show_camera() {
    // 显示摄像头图像
    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(frame.cols, frame.rows));
}
void Monitor::end_window() {
    // 结束窗口
    ImGui::End();
}

// 录制视频
void Monitor::record() {
    if (!is_recording_active()) {
        // 开始录制
        std::string filename = "recording_" + 
            std::to_string(std::time(nullptr)) + ".mp4";  // 使用时间戳作为文件名
        start_async_recording(saving_path + filename);
    } else {
        // 停止录制
        stop_async_recording();
    }
}

// 开始异步视频帧采集
void Monitor::start_frame_grabbing_function(double fps) {
    // 如果已经在采集，先停止
    if (is_frame_grabbing) {
        stop_frame_grabbing_function();
    }
    
    grabbing_fps = fps;
    
    // 重置停止标志
    stop_frame_grabbing = false;
    is_frame_grabbing = true;
    
    // 启动视频帧采集线程
    frame_grabber_thread = std::thread(&Monitor::frame_grabber_worker, this);
}

// 停止异步视频帧采集
void Monitor::stop_frame_grabbing_function() {
    if (is_frame_grabbing) {
        // 设置停止标志
        stop_frame_grabbing = true;
        
        // 等待线程结束
        if (frame_grabber_thread.joinable()) {
            frame_grabber_thread.join();
        }
        
        is_frame_grabbing = false;
    }
}

// 检查是否正在异步采集视频帧
bool Monitor::is_frame_grabbing_active() const {
    return is_frame_grabbing;
}

// 异步视频帧采集线程的工作函数
void Monitor::frame_grabber_worker() {
    if (camera == nullptr) {
        camera = new Camera(device_path);
    }
    
    // 计算帧之间的时间间隔（微秒）
    const int64_t frame_interval_us = static_cast<int64_t>(1000000.0 / grabbing_fps);
    
    // 用于帧率控制的时间点
    auto next_frame_time = std::chrono::high_resolution_clock::now();
    
    while (!stop_frame_grabbing) {
        // 获取一帧视频
        cv::Mat grabbed_frame;
        camera->capture_frame(grabbed_frame);
        
        {
            // 锁定以更新共享的frame
            std::lock_guard<std::mutex> lock(frame_mutex);
            frame = grabbed_frame.clone();
        }
        
        // 如果正在录制，将帧添加到队列
        if (is_recording) {
            std::unique_lock<std::mutex> lock(frame_mutex);
            
            // 限制队列大小，防止内存溢出
            if (frame_queue.size() > 90) {  // 增加到90帧缓冲，约3秒@30fps
                frame_queue.pop();
            }
            
            frame_queue.push(grabbed_frame.clone());
            lock.unlock();
            
            // 通知录制线程有新帧可用
            frame_cv.notify_one();
        }
        
        // 计算下一帧的时间点
        next_frame_time += std::chrono::microseconds(frame_interval_us);
        
        // 睡眠到下一帧的时间点
        auto now = std::chrono::high_resolution_clock::now();
        if (now < next_frame_time) {
            std::this_thread::sleep_until(next_frame_time);
        } else {
            // 如果已经错过了预定时间，调整下一帧时间点以避免积累延迟
            next_frame_time = now + std::chrono::microseconds(frame_interval_us);
        }
    }
}

void Monitor::capture_button(){
    if(ImGui::Button("Capture")) {
        //尚未做拍照逻辑

        // 捕获当前帧
        camera->capture_frame(frame);
        
        // 显示捕获的帧
        ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(frame.cols, frame.rows));
    }
}
void Monitor::record_button(){
    if(ImGui::Button("Record")) {
        // 录制视频
        record();
    }
}
bool Monitor::get_latest_recorded_frame(cv::Mat& out_frame) {
    std::lock_guard<std::mutex> lock(frame_mutex);
    if (!frame_queue.empty()) {
        out_frame = frame_queue.back().clone();
        return true;
    }
    return false;
}

std::vector<RecordInfo> Monitor::get_all_record_info() {
    // 从数据库中获取所有记录信息
    std::future<std::vector<std::vector<std::string>>> result;
    result = db->asyncQuery("SELECT video_path, start_time, end_time FROM record_info;");
    std::vector<std::vector<std::string>> db_infos = result.get();

    std::lock_guard<std::mutex> lock(record_info_mutex);
    std::vector<RecordInfo> infos;
    // 将数据库中的记录信息转换为RecordInfo对象
    for (const auto& db_info : db_infos) {
        RecordInfo info;
        info.filename = db_info[0];
        info.start_time = std::chrono::system_clock::time_point(std::chrono::seconds(std::stoll(db_info[1])));
        info.end_time = std::chrono::system_clock::time_point(std::chrono::seconds(std::stoll(db_info[2])));
        infos.push_back(info);
    }
    std::queue<RecordInfo> temp = record_info_queue;
    while (!temp.empty()) {
        infos.push_back(temp.front());
        temp.pop();
    }
    return infos;
}
std::string Monitor::convert_time_to_unix_timestamp(const std::chrono::system_clock::time_point& time_point){
    return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count());
}

std::vector<std::vector<std::string>> Monitor::search_video_from_timemap(std::string start_time, std::string end_time){
    std::future<std::vector<std::vector<std::string>>> result;
    result = db->asyncQuery("SELECT video_path FROM record_info WHERE start_time >= " + start_time + " AND end_time <= " + end_time +";");
    return result.get();
}

std::vector<std::string> Monitor::search_video_from_target_time(std::string target_time){
    std::future<std::vector<std::vector<std::string>>> result;
    result = db->asyncQuery("SELECT video_path FROM record_info WHERE start_time <= " + target_time + " AND end_time >= " + target_time + ";");
    // 处理查询结果
    std::vector<std::vector<std::string>> get=result.get();
    std::vector<std::string> found;
    if(get.empty()){
        std::cout << "没有找到符合条件的视频" << std::endl;
        return found;
    }
    return get.at(0);
}

std::vector<RecordInfo> Monitor::get_recent_record_info(int num_records){
    std::lock_guard<std::mutex> lock(record_info_mutex);
    std::vector<RecordInfo> infos;
    std::queue<RecordInfo> temp = record_info_queue;
    int count = 0;
    while (!temp.empty() && count < num_records) {
        infos.push_back(temp.front());
        temp.pop();
        count++;
    }
    if (count < num_records) {
        // 尝试从数据库中取出
        std::vector<std::vector<std::string>> db_infos = db->query("SELECT video_path, start_time, end_time FROM record_info ORDER BY start_time DESC LIMIT " + std::to_string(num_records - count) + ";");
        for (const auto& db_info : db_infos) {
            RecordInfo info;
            info.filename = db_info[0];
            info.start_time = std::chrono::system_clock::time_point(std::chrono::seconds(std::stoll(db_info[1])));
            info.end_time = std::chrono::system_clock::time_point(std::chrono::seconds(std::stoll(db_info[2])));
            infos.push_back(info);
        }
    }
    return infos;
}

bool Monitor::insert_record_info(const RecordInfo& record_info){
    // 将记录信息插入数据库
    return db->insertData("record_info", 
        "video_path, start_time, end_time", 
        "'" + record_info.filename + "', " + 
        convert_time_to_unix_timestamp(record_info.start_time) + ", " + 
        convert_time_to_unix_timestamp(record_info.end_time));
}
bool Monitor::delete_record_info(const std::string& filename){
    // 删除数据库中的记录
    return db->execute("DELETE FROM record_info WHERE video_path = '" + filename + "';");
}
