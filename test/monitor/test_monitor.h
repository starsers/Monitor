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
#ifndef TEST_DATABASE_H
#define TEST_DATABASE_H
// Database operations test class to be added to test_monitor.h
class DatabaseTests {
public:
    // Setup a test database 
    static void setupTestDatabase() {
        // Create a test database file
        SQLiteDatabase testDb("test_monitor.db");
        testDb.execute("DROP TABLE IF EXISTS videos");
        testDb.execute("CREATE TABLE IF NOT EXISTS videos (filename TEXT PRIMARY KEY, start_time TEXT, end_time TEXT)");
    }
    
    // Clean up test database
    static void cleanupTestDatabase() {
        // Remove test database file
        std::remove("test_monitor.db");
        std::cout << "Test database cleaned up." << std::endl;
    }
    
    // Test the insert_record_info and get_all_record_info functions
    static bool testInsertAndGetAllRecordInfo() {
        setupTestDatabase();
        
        // Create a monitor instance with test database
        Monitor monitor("/dev/null"); // Use null device to avoid real camera initialization
        // Create test record info
        RecordInfo record1;
        record1.filename = "test_video1.mp4";
        record1.start_time = std::chrono::system_clock::now();
        record1.end_time = std::chrono::system_clock::now() + std::chrono::seconds(10);
        
        RecordInfo record2;
        record2.filename = "test_video2.mp4";
        record2.start_time = std::chrono::system_clock::now() + std::chrono::seconds(20);
        record2.end_time = std::chrono::system_clock::now() + std::chrono::seconds(30);
        
        // Insert records
        monitor.insert_record_info(record1);
        monitor.insert_record_info(record2);
        
        // Get all records
        std::vector<RecordInfo> records = monitor.get_all_record_info();
        for(auto & record : records) {
            std::cout << "Filename: " << record.filename << ", Start Time: " 
                      << std::chrono::duration_cast<std::chrono::seconds>(record.start_time.time_since_epoch()).count() 
                      << ", End Time: " 
                      << std::chrono::duration_cast<std::chrono::seconds>(record.end_time.time_since_epoch()).count() 
                      << std::endl;
        }
        // Verify the records were inserted
        bool success = records.size() == 2;
        std::cout << "Number of records in database: " << records.size() << std::endl;
        if (!success) {
            std::cerr << "Expected 2 records, got " << records.size() << std::endl;
        } else {
            bool record1Found = false;
            bool record2Found = false;
            
            for (const auto& record : records) {
                if (record.filename == "test_video1.mp4") {
                    record1Found = true;
                } else if (record.filename == "test_video2.mp4") {
                    record2Found = true;
                }
            }
            
            success = record1Found && record2Found;
            if (!success) {
                std::cerr << "Not all records were found in the database" << std::endl;
            }
        }
        
        // Clean up inserted records
        monitor.delete_record_info("test_video1.mp4");
        monitor.delete_record_info("test_video2.mp4");
        
        cleanupTestDatabase();
        
        std::cout << "Insert and get all record info test " 
                 << (success ? "passed!" : "failed!") << std::endl;
        return success;
    }
    
    // Test the get_recent_record_info function
    static bool testGetRecentRecordInfo() {
        setupTestDatabase();
        
        Monitor monitor("/dev/null");
        
        // Insert multiple records
        for (int i = 0; i < 15; i++) {
            RecordInfo record;
            record.filename = "video_" + std::to_string(i) + ".mp4";
            record.start_time = std::chrono::system_clock::now() + std::chrono::seconds(i * 10);
            record.end_time = record.start_time + std::chrono::seconds(10);
            monitor.insert_record_info(record);
        }
        
        // Get recent 5 records
        std::vector<RecordInfo> recentRecords = monitor.get_recent_record_info(5);
        
        // Verify we got 5 records
        bool success = recentRecords.size() == 5;
        if (!success) {
            std::cerr << "Expected 5 recent records, got " << recentRecords.size() << std::endl;
        }
        
        // Clean up
        for (int i = 0; i < 15; i++) {
            monitor.delete_record_info("video_" + std::to_string(i) + ".mp4");
        }
        
        cleanupTestDatabase();
        
        std::cout << "Get recent record info test " 
                 << (success ? "passed!" : "failed!") << std::endl;
        return success;
    }
    
    // Test the search_video_from_timemap function
    static bool testSearchVideoFromTimemap() {
        setupTestDatabase();
        
        Monitor monitor("/dev/null");
        
        // Current time for reference
        auto now = std::chrono::system_clock::now();
        
        // Insert records with different time spans
        for (int i = 0; i < 5; i++) {
            RecordInfo record;
            record.filename = "timemap_video_" + std::to_string(i) + ".mp4";
            record.start_time = now + std::chrono::minutes(i * 10);
            record.end_time = record.start_time + std::chrono::minutes(5);
            monitor.insert_record_info(record);
        }
        
        // Convert time points to unix timestamps for searching
        auto startSearch = now + std::chrono::minutes(15);
        auto endSearch = now + std::chrono::minutes(35);
        
        std::string startTimeStr = Monitor::convert_time_to_unix_timestamp(startSearch);
        std::string endTimeStr = Monitor::convert_time_to_unix_timestamp(endSearch);
        
        // Search for videos in the time range
        auto results = monitor.search_video_from_timemap(startTimeStr, endTimeStr);
        for(auto & result : results) {
            std::cout << "Found video: " << result[0] << std::endl;
        }
        // We expect to find videos 1, 2, and 3 in this range
        bool success = results.size() == 3;
        
        // Clean up
        for (int i = 0; i < 5; i++) {
            monitor.delete_record_info("timemap_video_" + std::to_string(i) + ".mp4");
        }
        
        
        std::cout << "Search video from timemap test " 
                 << (success ? "passed!" : "failed!") << std::endl;
        return success;
    }
    
    // Test the search_video_from_target_time function
    static bool testSearchVideoFromTargetTime() {
        setupTestDatabase();
        
        Monitor monitor("/dev/null");
        
        // Current time for reference
        auto now = std::chrono::system_clock::now();
        
        // Insert records with different time spans
        for (int i = 0; i < 5; i++) {
            RecordInfo record;
            record.filename = "target_video_" + std::to_string(i) + ".mp4";
            record.start_time = now + std::chrono::minutes(i * 10);
            record.end_time = record.start_time + std::chrono::minutes(5);
            monitor.insert_record_info(record);
        }
        
        // Search for a video at a specific time - should find video_2
        auto targetTime = now + std::chrono::minutes(22); // Inside the time span of video_2
        std::string targetTimeStr = Monitor::convert_time_to_unix_timestamp(targetTime);
        
        auto results = monitor.search_video_from_target_time(targetTimeStr);
        
        bool success = results.size() == 1 && results[0] == "target_video_2.mp4";
        
        // Clean up
        for (int i = 0; i < 5; i++) {
            if(monitor.delete_record_info("target_video_" + std::to_string(i) + ".mp4")){
                std::cout << "Deleted target_video_" << i << ".mp4" << std::endl;
            } else {
                std::cerr << "Failed to delete target_video_" << i << ".mp4" << std::endl;
            };
        }
        
        
        std::cout << "Search video from target time test " 
                 << (success ? "passed!" : "failed!") << std::endl;
        return success;
    }
    
    // Test the delete_record_info function
    static bool testDeleteRecordInfo() {
        setupTestDatabase();
        
        Monitor monitor("/dev/null");
        
        // Insert a record
        RecordInfo record;
        record.filename = "delete_test_video.mp4";
        record.start_time = std::chrono::system_clock::now();
        record.end_time = record.start_time + std::chrono::seconds(10);
        monitor.insert_record_info(record);
        
        // Verify the record exists
        auto allRecords = monitor.get_all_record_info();
        bool recordExists = false;
        for (const auto& r : allRecords) {
            if (r.filename == "delete_test_video.mp4") {
                recordExists = true;
                break;
            }
        }
        
        if (!recordExists) {
            std::cerr << "Record was not inserted correctly for deletion test" << std::endl;
            cleanupTestDatabase();
            return false;
        }
        
        // Delete the record
        monitor.delete_record_info("delete_test_video.mp4");
        
        // Verify the record was deleted
        allRecords = monitor.get_all_record_info();
        bool recordDeleted = true;
        for (const auto& r : allRecords) {
            if (r.filename == "delete_test_video.mp4") {
                recordDeleted = false;
                break;
            }
        }
        
        cleanupTestDatabase();
        
        std::cout << "Delete record info test " 
                 << (recordDeleted ? "passed!" : "failed!") << std::endl;
        return recordDeleted;
    }
    static bool testInsert(){
        Monitor monitor("/dev/null");
        RecordInfo record;
        record.filename = "test_video.mp4";
        record.start_time = std::chrono::system_clock::now();
        record.end_time = record.start_time + std::chrono::seconds(10);
        if(monitor.insert_record_info(record)){
            std::cout << "Inserted record: " << record.filename << std::endl;
        } else {
            std::cerr << "Failed to insert record: " << record.filename << std::endl;
            return false;
        };
        
        auto allRecords = monitor.get_all_record_info();
        for(auto& r : allRecords){
            std::cout << "Record filename: " << r.filename << std::endl;
        }
        bool recordExists = false;
        for (const auto& r : allRecords) {
            if (r.filename == "test_video.mp4") {
                recordExists = true;
                break;
            }
        }
        
        
        std::cout << "Insert test " 
                 << (recordExists ? "passed!" : "failed!") << std::endl;
        return recordExists;
    }

    // Run all database tests
    static void runAllTests() {
        testInsertAndGetAllRecordInfo();
        testGetRecentRecordInfo();
        testSearchVideoFromTimemap();
        testSearchVideoFromTargetTime();
        testDeleteRecordInfo();
        
        std::cout << "All database tests completed." << std::endl;
    }
};
#endif // TEST_DATABASE_H