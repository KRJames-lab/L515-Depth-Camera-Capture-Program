#pragma once

#include <librealsense2/rs.hpp>
#include <string>
#include <memory>
#include "load_config.h"

// RealSense camera related class
class Camera {
public:
    // Constructor
    Camera(const LoadConfig& config);
    
    // Destructor
    ~Camera();
    
    // Initialize camera
    bool initialize();
    
    // Capture frames
    bool captureFrames();
    
    // Return depth scale
    float getDepthScale() const { return depth_scale; }
    
    // Frame accessors
    rs2::frameset getFrames() const { return frames; }
    rs2::frame getDepthFrame() const { return depth_frame; }
    rs2::frame getColorFrame() const { return color_frame; }
    rs2::frame getInfraredFrame() const { return ir_frame; }
    rs2::frame getConfidenceFrame() const { return confidence_frame; }
    
    // Return frame dimensions
    int getDepthWidth() const { return depth_frame.as<rs2::video_frame>().get_width(); }
    int getDepthHeight() const { return depth_frame.as<rs2::video_frame>().get_height(); }
    
    // Check if device is L515 model
    bool isL515() const { return is_l515; }
    
private:
    // Configuration reference
    const LoadConfig& config;
    
    // RealSense objects
    rs2::pipeline pipe;
    rs2::config rs_config;
    rs2::context ctx;
    
    // Frame objects - initialized with default values
    rs2::frameset frames;
    rs2::frame depth_frame;  // Changed from rs2::depth_frame to rs2::frame
    rs2::frame color_frame;  // Changed from rs2::video_frame to rs2::frame
    rs2::frame ir_frame;     // Changed from rs2::video_frame to rs2::frame
    rs2::frame confidence_frame;
    
    // Depth scale
    float depth_scale = 0.0f;
    
    // Flag indicating if device is L515
    bool is_l515 = false;
    bool pipeline_started = false; // Flag to track if pipeline was started
    
    // Check for L515 device (private helper function)
    bool checkL515Device(const rs2::device& dev);

    // Helper function to apply sensor options
    template <typename T>
    void applyOption(rs2::sensor& sensor, rs2_option option, T value, const std::string& option_name);
}; 