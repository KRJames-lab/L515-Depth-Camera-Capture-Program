#include "camera.h"
#include <iostream>

Camera::Camera(const LoadConfig& config) : config(config) {
}

Camera::~Camera() {
    try {
        pipe.stop();
    } catch (const rs2::error& e) {
        std::cerr << "Error shutting down camera: " << e.what() << std::endl;
    }
}

// Camera Initialize
bool Camera::initialize() {
    try {
        // Check connected device
        auto devices = ctx.query_devices();
        if (devices.size() == 0) {
            std::cerr << "There is no RealSense device connected.\n";
            return false;
        }
        
        // Check L515 model
        is_l515 = false;
        for (const auto& dev : devices) {
            if (checkL515Device(dev)) {
                is_l515 = true;
                rs_config.enable_device(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
                break;
            }
        }
        
        if (!is_l515) {
            std::cout << "Warning: L515 camera not detected. Using other RealSense device.\n";
        }

        // Set depth stream
        rs_config.enable_stream(RS2_STREAM_DEPTH, 
                               config.getDepthWidth(), 
                               config.getDepthHeight(), 
                               config.getDepthRs2Format(), 
                               config.getFps());
        
        std::cout << "Depth stream enabled: " << config.getDepthWidth() << "x" << config.getDepthHeight() 
                  << " @ " << config.getFps() << "fps, format: " << config.getDepthFormat() << std::endl;
        
        // If color image is needed, enable color stream
        if (config.saveColorImage()) {
            rs_config.enable_stream(RS2_STREAM_COLOR, 
                                   config.getColorWidth(), 
                                   config.getColorHeight(), 
                                   config.getColorRs2Format(), 
                                   config.getFps());
            
            std::cout << "Color stream enabled: " << config.getColorWidth() << "x" << config.getColorHeight() 
                      << " @ " << config.getFps() << "fps, format: " << config.getColorFormat() << std::endl;
        }
        
        // If infrared stream is needed, enable it
        if (config.enableInfrared()) {
            rs_config.enable_stream(RS2_STREAM_INFRARED, 
                                   config.getDepthWidth(), 
                                   config.getDepthHeight(), 
                                   RS2_FORMAT_Y8, 
                                   config.getFps());
            
            std::cout << "Infrared stream enabled: " << config.getDepthWidth() << "x" << config.getDepthHeight() 
                      << " @ " << config.getFps() << "fps, format: Y8" << std::endl;
        }
        
        // If confidence stream is needed, enable it (L515 only)
        if (config.enableConfidence() && is_l515) {
            rs_config.enable_stream(RS2_STREAM_CONFIDENCE, 
                                   config.getDepthWidth(), 
                                   config.getDepthHeight(), 
                                   RS2_FORMAT_RAW8, 
                                   config.getFps());
            
            std::cout << "Confidence stream enabled: " << config.getDepthWidth() << "x" << config.getDepthHeight() 
                      << " @ " << config.getFps() << "fps, format: RAW8" << std::endl;
        }

        // Start streaming
        rs2::pipeline_profile profile = pipe.start(rs_config);
        
        // Get depth scale
        depth_scale = profile.get_device().first<rs2::depth_sensor>().get_depth_scale();
        std::cout << "Depth scale: " << depth_scale << " meters/unit" << std::endl;
        
        // Wait for 1 second and capture frames (camera stabilization)
        for (int i = 0; i < config.getFps(); i++) {
            pipe.wait_for_frames();
        }
        
        return true;
    }
    catch (const rs2::error& e) {
        std::cerr << "RealSense initialization error: " << e.what() << " (" << e.get_failed_function() << ")" << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return false;
    }
}

// Capture frames
bool Camera::captureFrames() {
    try {
        // Capture frames
        frames = pipe.wait_for_frames();
        depth_frame = frames.first_or_default(RS2_STREAM_DEPTH);
        
        if (!depth_frame) {
            std::cerr << "Failed to capture depth frame\n";
            return false;
        }
        
        // If color frame is needed, capture it
        if (config.saveColorImage()) {
            color_frame = frames.first_or_default(RS2_STREAM_COLOR);
            if (!color_frame) {
                std::cerr << "Failed to capture color frame\n";
                return false;
            }
        }
        
        // If infrared frame is needed, capture it
        if (config.enableInfrared() && config.saveInfrared()) {
            ir_frame = frames.first_or_default(RS2_STREAM_INFRARED);
            if (!ir_frame) {
                std::cerr << "Failed to capture infrared frame\n";
                // Infrared can continue even if it fails
            }
        }
        
        // If confidence frame is needed, capture it (L515 only)
        if (config.enableConfidence() && is_l515) {
            confidence_frame = frames.first_or_default(RS2_STREAM_CONFIDENCE);
            if (!confidence_frame) {
                std::cerr << "Failed to capture confidence frame\n";
                // Confidence can continue even if it fails
            }
        }
        
        return true;
    }
    catch (const rs2::error& e) {
        std::cerr << "RealSense frame capture error: " << e.what() << " (" << e.get_failed_function() << ")" << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Frame capture error: " << e.what() << std::endl;
        return false;
    }
}

// Check L515 model function
bool Camera::checkL515Device(const rs2::device& dev) {
    if (std::string(dev.get_info(RS2_CAMERA_INFO_NAME)).find("L515") != std::string::npos) {
        std::cout << "L515 camera detected.\n";
        return true;
    }
    return false;
} 