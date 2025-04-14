#pragma once

#include <string>
#include <map>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <librealsense2/rs.hpp>

// Configuration and settings related class
class LoadConfig {
public:
    // Default constructor
    LoadConfig();
    
    // Load from configuration file
    bool loadFromFile(const std::string& filename);
    
    // Configuration accessors
    int getDepthWidth() const { return depth_width; }
    int getDepthHeight() const { return depth_height; }
    int getColorWidth() const { return color_width; }
    int getColorHeight() const { return color_height; }
    int getFps() const { return fps; }
    std::string getOutputDir() const { return output_dir; }
    std::string getDepthFormat() const { return depth_format; }
    std::string getColorFormat() const { return color_format; }
    int getFloatPrecision() const { return float_precision; }
    int getNumThreads() const { return num_threads; }
    bool useDirectAccess() const { return use_direct_access; }
    bool enableInfrared() const { return enable_infrared; }
    bool enableConfidence() const { return enable_confidence; }
    bool saveColorImage() const { return save_color_image; }
    bool saveDepthMap() const { return save_depth_map; }
    bool saveDepthRaw() const { return save_depth_raw; }
    bool saveDepthBinary() const { return save_depth_binary; }
    bool saveInfrared() const { return save_infrared; }
    int getCsvSamplingStep() const { return csv_sampling_step; }
    std::string getBinaryFilename() const { return binary_filename; }
    
    // rs2_format conversion
    rs2_format getDepthRs2Format() const;
    rs2_format getColorRs2Format() const;
    
    // Stream format validation
    static bool isValidFormat(const std::string& format, const std::string& stream_type);
    
    // Format map access
    static const std::map<std::string, rs2_format>& getFormatMap() { return format_map; }

private:
    // Camera settings
    int depth_width = 1024;
    int depth_height = 768;
    int color_width = 1920;
    int color_height = 1080;
    int fps = 30;
    std::string output_dir = "./";
    
    // Format settings
    std::string depth_format = "Z16";    // Default depth format
    std::string color_format = "RGB8";   // Default color format
    
    // Data format settings
    int float_precision = 32;  // Floating point precision: 32 or 64 bits
    
    // Performance settings
    int num_threads = 0;      // OpenMP thread count (0 = auto)
    bool use_direct_access = true;  // Use direct memory access
    
    // Additional stream enable options
    bool enable_infrared = false;       // Infrared stream
    bool enable_confidence = false;     // Confidence stream
    
    // Output settings
    bool save_color_image = false;
    bool save_depth_map = false;
    bool save_depth_raw = false;
    bool save_depth_binary = true;
    bool save_infrared = false;
    int csv_sampling_step = 5;
    std::string binary_filename = "depth_data.bin";
    
    // Map to convert stream format strings to rs2_format
    static std::map<std::string, rs2_format> format_map;
}; 