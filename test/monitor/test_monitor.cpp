#include "test_monitor.h"
int main() {
    // Run all tests
    // MonitorTests::runAllTests();
    // int frameCount = 100; // Number of frames to capture
    // std::string frameCountStr = std::to_string(frameCount);
    // std::cout << "Press Enter to start capturing " << frameCountStr << " frames..." << std::endl;
    // char tmp[100];
    // std::cin.getline(tmp, 100);// 输入任意字符
    // if(!tmp[0]){
    //     frameCount = std::atoi(tmp);
    //     std::cout << "Capturing " << frameCount << " frames..." << std::endl;
    // }
    // Monitor monitor;
    // monitor.capture();
    // MonitorTests::convertMonitorFramesToVideo(monitor, frameCount, "test_video.mp4");
    // DatabaseTests::testInsert();
    // DatabaseTests::testInsertAndGetAllRecordInfo();
    // DatabaseTests::testGetRecentRecordInfo();
    // DatabaseTests::testSearchVideoFromTimemap();
    DatabaseTests::testSearchVideoFromTargetTime();
    DatabaseTests::testDeleteRecordInfo();

    return 0;
}