// filepath: include/monitor/test_monitor.h
#ifndef TEST_MONITOR_H
#define TEST_MONITOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include "monitor.h"

class MonitorTests {
    Camera* camera;
public:
    MonitorTests() {
        // camera = new Camera();
    }
    void init() {
        camera = new Camera();
    }
    ~MonitorTests() {
        if (camera != nullptr) {
            delete camera;
            camera = nullptr;
        }
    }
    // Test basic cv::Mat operations
    static bool testBasicMatOperations() {
        cv::Mat testFrame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
        
        // Test frame dimensions
        assert(testFrame.rows == 480);
        assert(testFrame.cols == 640);
        assert(testFrame.channels() == 3);
        
        // Test pixel access
        testFrame.at<cv::Vec3b>(100, 100) = cv::Vec3b(255, 0, 0); // Set blue pixel
        cv::Vec3b pixel = testFrame.at<cv::Vec3b>(100, 100);
        assert(pixel[0] == 255); // Blue channel
        
        // Test frame copy
        cv::Mat frameCopy = testFrame.clone();
        assert(frameCopy.size() == testFrame.size());
        assert(frameCopy.type() == testFrame.type());
        
        std::cout << "Basic Mat operations test passed!" << std::endl;
        return true;
    }
    
    // Test converting a single frame to compressed format (JPEG)
    static bool testFrameCompression() {
        cv::Mat testFrame(480, 640, CV_8UC3, cv::Scalar(0, 128, 255));
        
        // Test JPEG compression
        std::vector<uchar> buffer;
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 90};
        cv::imencode(".jpg", testFrame, buffer, params);
        
        // Verify compression worked
        assert(!buffer.empty());
        assert(buffer.size() < testFrame.total() * testFrame.elemSize());
        
        // Test decompression
        cv::Mat decodedFrame = cv::imdecode(buffer, cv::IMREAD_COLOR);
        assert(!decodedFrame.empty());
        assert(decodedFrame.size() == testFrame.size());
        
        std::cout << "Frame compression test passed!" << std::endl;
        return true;
    }
    
    // Test converting frames to video file
    static bool testFramesToVideo() {
        // Create test frames
        std::vector<cv::Mat> frames;
        for (int i = 0; i < 30; i++) {
            cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(i * 8, 255 - i * 8, 128));
            
            // Add some text to differentiate frames
            cv::putText(frame, "Frame " + std::to_string(i), 
                        cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                        cv::Scalar(255, 255, 255), 2);
                        
            frames.push_back(frame);
        }
        
        // Create video writer
        cv::VideoWriter writer;
        std::string filename = "test_video.mp4";
        int codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');  // H.264 codec
        double fps = 30.0;
        cv::Size frameSize(640, 480);
        
        writer.open(filename, codec, fps, frameSize);
        
        if (!writer.isOpened()) {
            std::cerr << "Could not open video file for writing" << std::endl;
            return false;
        }
        
        // Write frames to video
        for (const auto& frame : frames) {
            writer.write(frame);
        }
        
        writer.release();
        
        // Verify file was created
        cv::VideoCapture cap(filename);
        bool success = cap.isOpened();
        cap.release();
        
        if (success) {
            std::cout << "Frames to video test passed!" << std::endl;
        }
        
        return success;
    }
    
    // Helper function to convert Monitor's frames to compressed video
    static void convertMonitorFramesToVideo(Monitor& monitor, int frameCount, 
                                           const std::string& filename, 
                                           double fps = 30.0,
                                           int codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1')) {
        // Initialize video writer
        cv::VideoWriter writer;
        bool isColor = true;
        
        // Capture first frame to get dimensions
        monitor.capture();
        cv::Size frameSize(monitor.frame.cols, monitor.frame.rows);
        
        writer.open(filename, codec, fps, frameSize, isColor);
        
        if (!writer.isOpened()) {
            std::cerr << "Could not open video file for writing" << std::endl;
            return;
        }
        
        // Capture and write frames
        for (int i = 0; i < frameCount; i++) {
            monitor.capture();
            writer.write(monitor.frame);
        }
        
        writer.release();
        std::cout << "Video saved to: " << filename << std::endl;
    }
    
    // Function that demonstrates different codecs
    static void demonstrateVideoCodecs() {
        cv::Mat testFrame(480, 640, CV_8UC3, cv::Scalar(0, 0, 255));
        std::vector<cv::Mat> frames;
        
        // Generate 30 test frames
        for (int i = 0; i < 30; i++) {
            cv::Mat frame = testFrame.clone();
            cv::putText(frame, "Frame " + std::to_string(i), 
                       cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                       cv::Scalar(255, 255, 255), 2);
            frames.push_back(frame);
        }
        
        // Common codecs and their FourCC codes
        struct CodecInfo {
            std::string name;
            int fourcc;
            std::string extension;
        };
        
        std::vector<CodecInfo> codecs = {
            {"MPEG-4", cv::VideoWriter::fourcc('M', 'P', '4', 'V'), ".mp4"},
            {"H.264", cv::VideoWriter::fourcc('a', 'v', 'c', '1'), ".mp4"},
            {"MJPG", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), ".avi"},
            {"XVID", cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), ".avi"}
        };
        
        for (const auto& codec : codecs) {
            std::string filename = "test_" + codec.name + codec.extension;
            cv::VideoWriter writer(filename, codec.fourcc, 30.0, cv::Size(640, 480));
            
            if (!writer.isOpened()) {
                std::cout << "Warning: Could not open codec " << codec.name << std::endl;
                continue;
            }
            
            for (const auto& frame : frames) {
                writer.write(frame);
            }
            
            writer.release();
            std::cout << "Created video with codec " << codec.name << ": " << filename << std::endl;
        }
    }
    
    // Run all tests
    static void runAllTests() {
        testBasicMatOperations();
        testFrameCompression();
        testFramesToVideo();
        demonstrateVideoCodecs();
    }
};

#endif // TEST_MONITOR_H