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
    bool saveDepthCsv() const { return save_depth_csv; }
    int getCsvSamplingStep() const { return csv_sampling_step; }
    std::string getBinaryFilename() const { return binary_filename; }

    
    // rs2_format conversion
    rs2_format getDepthRs2Format() const;
    rs2_format getColorRs2Format() const;
    
    // Stream format validation
    static bool isValidFormat(const std::string& format, const std::string& stream_type);
    
    // Format map access
    static const std::map<std::string, rs2_format>& getFormatMap() { return format_map; }

    // Advanced Camera Option Accessors
    float getLaserPower() const { return laser_power; }
    int getConfidenceThreshold() const { return confidence_threshold; }
    float getMinDistance() const { return min_distance; }
    float getReceiverGain() const { return receiver_gain; }
    float getPostProcessingSharpening() const { return post_processing_sharpening; }
    float getNoiseFiltering() const { return noise_filtering; }
    bool getInvalidationBypass() const { return invalidation_bypass; }
    bool getEnableErrorPolling() const { return enable_error_polling; }
    int getInterCamSyncMode() const { return inter_cam_sync_mode; }
    bool getFreefallDetectionEnabled() const { return freefall_detection_enabled; }
    int getEmitterEnabled() const { return emitter_enabled; }
    int getVisualPreset() const { return visual_preset; }
    bool getGlobalTimeEnabled() const { return global_time_enabled; }

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
    bool save_depth_csv = false;
    int csv_sampling_step = 5;
    std::string binary_filename = "depth_data.bin";
    
    // Map to convert stream format strings to rs2_format
    static std::map<std::string, rs2_format> format_map;

    // --- Advanced Camera Options ---
    // Note: Default values are examples, check SDK/L515 specs for actual defaults/ranges
    float laser_power = 100.0f;
    int confidence_threshold = 1;
    float min_distance = 0.0f;          // Often read-only, SDK sets this
    float receiver_gain = 16.0f;        // Default gain
    float post_processing_sharpening = 1.0f; // Default sharpening level (0-?)
    float noise_filtering = 4.0f;     // Default noise filter level (0-?)
    bool invalidation_bypass = false;   // Default bypass state
    bool enable_error_polling = true;   // Default error polling state
    int inter_cam_sync_mode = 0;      // 0: Default, 1: Master, 2: Slave...
    bool freefall_detection_enabled = true; // Default freefall detection state
    int emitter_enabled = 1;          // 0: Off, 1: On, 2: Auto
    int visual_preset = 0;            // 0: Custom, 1: Max Range, 2: Short Range...
    bool global_time_enabled = true;  // Default global time state

    // Options to investigate further for L515 availability/control:
    // Pre Processing Sharpening, Depth Units (Read-Only), Depth Offset,
    // Camera Accuracy Health Enabled, Ambient Light, Zero Order Enabled
}; 