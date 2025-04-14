#pragma once

#include <opencv2/opencv.hpp>
#include <chrono>
#include <string>
#include <omp.h>
#include "load_config.h"
#include "camera.h"

// Depth data processing related class
class Processor {
public:
    // Constructor
    Processor(const LoadConfig& config, const Camera& camera);
    
    // Process depth frame
    bool processDepthFrame();
    
    // Process color frame
    bool processColorFrame();
    
    // Process infrared frame
    bool processInfraredFrame();
    
    // Save depth data
    bool saveDepthData();
    
    // Processed data accessors
    const cv::Mat& getDepthMeters() const { return depth_meters; }
    const cv::Mat& getDepthRaw() const { return depth_raw; }
    const cv::Mat& getColorImage() const { return color_image; }
    const cv::Mat& getInfraredImage() const { return ir_image; }
    
private:
    // Create result directory
    void createResultDirectory();
    
    // Configuration and camera references
    const LoadConfig& config;
    const Camera& camera;
    
    // Output directory path
    std::string output_dir;
    
    // Processed data
    cv::Mat depth_meters;  // Depth map in meters
    cv::Mat depth_raw;     // Raw depth data
    cv::Mat color_image;   // Color image
    cv::Mat ir_image;      // Infrared image
    
    // Save depth data to binary file
    template <typename T>
    bool saveDepthBinary(const cv::Mat& depth_data, const std::string& filename);
    
    // Print depth map information
    void printDepthInfo() const;
}; 