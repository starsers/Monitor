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

    // 检查并执行定时任务
    check_and_execute_scheduled_tasks();
}

void Monitor::display_camera_frame(const cv::Mat& frame) {
    // 更新纹理内容
    update_texture(frame);

    // 使用 ImGui 显示纹理
    start_window();
    
    // 明确的布局开始
    ImGui::BeginGroup();
    
    // 摄像头显示区域
    ImGui::BeginChild("CameraView", ImVec2(frame.cols, frame.rows), true);
    show_camera();
    ImGui::EndChild();
    
    ImGui::EndGroup();
    
    // 控制按钮区域
    ImGui::SameLine();
    ImGui::BeginGroup();
    
    record_button();
    ImGui::SameLine();
    capture_button();
    
    ImGui::Separator();
    
    // 定时录制控制
    if (ImGui::CollapsingHeader("Scheduled Recording")) {
        schedule_record_button();
        ImGui::Separator();
        schedule_stop_button();
    }
    
    // 视频搜索界面
    if (ImGui::CollapsingHeader("Video Search")) {
        video_search_ui();
    }
    ImGui::EndGroup();
    end_window();
}

void Monitor::display(){
    std::lock_guard<std::mutex> lock(frame_mutex);
    if (!frame.empty()) {
        // 确保纹理有效
        if (textureID == 0) {
            std::cout << "Re-initializing texture for display" << std::endl;
            init_texture(frame.cols, frame.rows);
        }
        display_camera_frame(frame);
    }
    else {
        std::cerr << "Frame is empty in display()" << std::endl;
    }
}
void Monitor::display_dynamic(){
    bool frame_updated = false;
    
    if (is_recording) {
        // 录制时，从队列取最新帧
        cv::Mat latest;
        if (get_latest_recorded_frame(latest)) {
            std::cout << "Got latest frame from recording queue" << std::endl;
            frame = latest.clone();
            frame_updated = true;
        }
    } else {
        // 非录制时，直接采集新帧
        // std::cout << "Attempting to capture new frame" << std::endl;
        if (camera && camera->is_opened()) {
            camera->capture_frame(frame);
            frame_updated = true;
        } else {
            std::cerr << "Camera not available or not opened!" << std::endl;
            // 尝试重新初始化摄像头
            if (camera == nullptr) {
                camera = new Camera(device_path);
            }
        }
    }
    
    // if (frame_updated) {
    //     std::cout << "Frame updated, size: " << frame.cols << "x" << frame.rows << std::endl;
    // } else {
    //     std::cout << "No new frame captured" << std::endl;
    // }
    
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

        // 存储数据数据到数据库
        std::lock_guard<std::mutex> lock(record_info_mutex);
        while (!record_info_queue.empty()) {
            RecordInfo record_info = record_info_queue.front();
            if(insert_record_info(record_info)){
                    record_info_queue.pop();
                    std::cout << "录制信息已保存到数据库" << std::endl;
            }else{
                std::cerr << "录制信息保存到数据库失败" << std::endl;
                break;
            }
        }
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
// 在 record_button() 函数之后添加以下函数

void Monitor::schedule_record_button() {
    static char start_time[64] = "";
    ImGui::InputText("Start Recording Time (YYYY-MM-DD HH:MM:SS)", start_time, 64);
    
    if (ImGui::Button("Schedule Recording Start")) {
        // 解析时间字符串
        std::tm tm = {};
        std::istringstream ss(start_time);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        
        if (ss.fail()) {
            std::cerr << "Invalid time format. Please use YYYY-MM-DD HH:MM:SS" << std::endl;
        } else {
            std::time_t time_t = std::mktime(&tm);
            scheduled_record_time = std::chrono::system_clock::from_time_t(time_t);
            schedule_recording = true;
            
            // 显示确认消息
            auto now = std::chrono::system_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::seconds>(scheduled_record_time - now).count();
            std::cout << "Recording scheduled to start in " << diff << " seconds" << std::endl;
        }
    }
    
    if (schedule_recording) {
        // 显示倒计时
        auto now = std::chrono::system_clock::now();
        auto diff = scheduled_record_time - now;
        if (diff.count() > 0) {
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
            ImGui::Text("Recording will start in %lld seconds", seconds);
        }
    }
}

void Monitor::schedule_stop_button() {
    static char stop_time[64] = "";
    ImGui::InputText("Stop Recording Time (YYYY-MM-DD HH:MM:SS)", stop_time, 64);
    
    if (ImGui::Button("Schedule Recording Stop")) {
        // 解析时间字符串
        std::tm tm = {};
        std::istringstream ss(stop_time);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        
        if (ss.fail()) {
            std::cerr << "Invalid time format. Please use YYYY-MM-DD HH:MM:SS" << std::endl;
        } else {
            std::time_t time_t = std::mktime(&tm);
            scheduled_stop_time = std::chrono::system_clock::from_time_t(time_t);
            schedule_stop_recording = true;
            
            // 显示确认消息
            auto now = std::chrono::system_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::seconds>(scheduled_stop_time - now).count();
            std::cout << "Recording scheduled to stop in " << diff << " seconds" << std::endl;
        }
    }
    
    if (schedule_stop_recording) {
        // 显示倒计时
        auto now = std::chrono::system_clock::now();
        auto diff = scheduled_stop_time - now;
        if (diff.count() > 0) {
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
            ImGui::Text("Recording will stop in %lld seconds", seconds);
        }
    }
}

bool Monitor::check_and_execute_scheduled_tasks() {
    auto now = std::chrono::system_clock::now();
    bool executed = false;
    
    // 检查是否需要开始录制
    if (schedule_recording && now >= scheduled_record_time) {
        // 如果未在录制，开始录制
        if (!is_recording_active()) {
            std::string filename = "scheduled_recording_" + 
                std::to_string(std::time(nullptr)) + ".mp4";
            start_async_recording(saving_path + filename);
            std::cout << "Scheduled recording started" << std::endl;
        }
        schedule_recording = false;
        executed = true;
    }
    
    // 检查是否需要停止录制
    if (schedule_stop_recording && now >= scheduled_stop_time) {
        // 如果正在录制，停止录制
        if (is_recording_active()) {
            stop_async_recording();
            std::cout << "Scheduled recording stopped" << std::endl;
        }
        schedule_stop_recording = false;
        executed = true;
    }
    
    return executed;
}

void Monitor::video_search_ui() {
    ImGui::Text("Video Search");
    ImGui::Separator();
    
    // 按时间范围搜索
    static char start_time[64] = "";
    static char end_time[64] = "";
    ImGui::Text("Search by Time Range:");
    ImGui::InputText("Start Time (YYYY-MM-DD HH:MM:SS)", start_time, 64);
    ImGui::InputText("End Time (YYYY-MM-DD HH:MM:SS)", end_time, 64);
    
    if (ImGui::Button("Search Videos in Range")) {
        // 解析时间字符串
        std::tm start_tm = {}, end_tm = {};
        std::istringstream start_ss(start_time), end_ss(end_time);
        start_ss >> std::get_time(&start_tm, "%Y-%m-%d %H:%M:%S");
        end_ss >> std::get_time(&end_tm, "%Y-%m-%d %H:%M:%S");
        
        if (start_ss.fail() || end_ss.fail()) {
            std::cerr << "Invalid time format. Please use YYYY-MM-DD HH:MM:SS" << std::endl;
        } else {
            std::time_t start_time_t = std::mktime(&start_tm);
            std::time_t end_time_t = std::mktime(&end_tm);
            
            // 转换为 Unix 时间戳字符串
            search_start_time_str = std::to_string(start_time_t);
            search_end_time_str = std::to_string(end_time_t);
            
            // 执行查询
            auto search_result = search_video_from_timemap(search_start_time_str, search_end_time_str);
            search_results.clear();
            for (const auto& result : search_result) {
                if (!result.empty()) {
                    search_results.push_back(result[0]);
                }
            }
            
            show_search_results = true;
        }
    }
    
    ImGui::Separator();
    
    // 按特定时间点搜索
    static char target_time[64] = "";
    ImGui::Text("Search by Specific Time Point:");
    ImGui::InputText("Time Point (YYYY-MM-DD HH:MM:SS)", target_time, 64);
    
    if (ImGui::Button("Find Video at Time Point")) {
        // 解析时间字符串
        std::tm target_tm = {};
        std::istringstream target_ss(target_time);
        target_ss >> std::get_time(&target_tm, "%Y-%m-%d %H:%M:%S");
        
        if (target_ss.fail()) {
            std::cerr << "Invalid time format. Please use YYYY-MM-DD HH:MM:SS" << std::endl;
        } else {
            std::time_t target_time_t = std::mktime(&target_tm);
            search_target_time_str = std::to_string(target_time_t);
            
            // 执行查询
            search_results = search_video_from_target_time(search_target_time_str);
            show_search_results = true;
        }
    }
    
    // 显示搜索结果
    if (show_search_results) {
        ImGui::Separator();
        ImGui::Text("Search Results:");
        
        if (search_results.empty()) {
            ImGui::Text("No videos found matching criteria");
        } else {
            for (size_t i = 0; i < search_results.size(); i++) {
                ImGui::Text("%zu. %s", i + 1, search_results[i].c_str());
                
                // 添加播放按钮
                ImGui::SameLine();
                if (ImGui::Button(("Play##" + std::to_string(i)).c_str())) {
                    // 调用系统命令播放视频
                    std::string command = "xdg-open \"" + search_results[i] + "\" &";
                    std::system(command.c_str());
                }
                
                // 添加删除按钮
                ImGui::SameLine();
                if (ImGui::Button(("Delete##" + std::to_string(i)).c_str())) {
                    if (delete_record_info(search_results[i])) {
                        std::cout << "Record deleted from database: " << search_results[i] << std::endl;
                        // 可以选择是否物理删除文件
                        // std::remove(search_results[i].c_str());
                        search_results.erase(search_results.begin() + i);
                        i--; // 调整索引以避免跳过下一个元素
                    }
                }
            }
        }
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
