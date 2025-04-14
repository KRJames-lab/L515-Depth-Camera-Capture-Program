#include "camera.h"
#include <iostream>
#include <vector> // Needed for querying sensors

Camera::Camera(const LoadConfig& config) : config(config) {
}

Camera::~Camera() {
    try {
        if (pipeline_started) { // Check if pipeline was started using the flag
             pipe.stop();
        }
    } catch (const rs2::error& e) {
        std::cerr << "Error shutting down camera: " << e.what() << std::endl;
    } catch (...) {
        // Catch any other exceptions during cleanup
        std::cerr << "Unknown error during camera shutdown." << std::endl;
    }
}

// Helper function to apply options
template <typename T>
void Camera::applyOption(rs2::sensor& sensor, rs2_option option, T value, const std::string& option_name) {
    try {
        if (sensor.supports(option)) {
            // Check if option is read-only BEFORE trying to set it
             if (sensor.is_option_read_only(option)) {
                 std::cout << "  Option '" << option_name << "' is read-only." << std::endl;
             } else {
                 // Use as_option() to handle potential type issues more robustly if needed,
                 // but set_option(float) is generally used. Cast booleans to float (0.0/1.0).
                 float float_value;
                 if constexpr (std::is_same_v<T, bool>) {
                     float_value = value ? 1.0f : 0.0f;
                 } else {
                     float_value = static_cast<float>(value);
                 }

                 sensor.set_option(option, float_value);
                 // Optionally, read back the value to confirm
                 // float read_value = sensor.get_option(option);
                 // std::cout << "  Set " << option_name << " to " << value << " (Read back: " << read_value << ")" << std::endl;
                 std::cout << "  Set " << option_name << " to " << value << std::endl;
             }
        } else {
            std::cout << "  Option '" << option_name << "' not supported by this sensor." << std::endl;
        }
    } catch (const rs2::invalid_value_error& e) {
        std::cerr << "  Failed to set " << option_name << " to " << value << ". Invalid value: " << e.what() << std::endl;
    } catch (const rs2::wrong_api_call_sequence_error& e) {
         std::cerr << "  Failed to set " << option_name << ". API call sequence error: " << e.what() << std::endl;
    } catch (const rs2::error& e) {
        std::cerr << "  Failed to set " << option_name << ": " << e.what() << std::endl;
    } catch (const std::exception& e) {
         std::cerr << "  Error setting " << option_name << ": " << e.what() << std::endl;
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

        // Start streaming - BEFORE setting most options
        // Some options might require the stream to be active, others must be set before.
        // It's generally safer to set options AFTER starting the pipeline and getting the sensor.
        rs2::pipeline_profile profile = pipe.start(rs_config);
        pipeline_started = true; // Set flag after successful start
        rs2::device device = profile.get_device();

        // --- Apply Advanced Camera Options ---
        // Get the depth sensor (L515 options are primarily on the depth sensor)
        std::vector<rs2::sensor> sensors = device.query_sensors();
        rs2::sensor depth_sensor; // Use a generic sensor object
        for(const auto& s : sensors) {
             // Check if it's a depth sensor - safer than just device.first<rs2::depth_sensor>()
             if (s.is<rs2::depth_sensor>()) {
                 depth_sensor = s;
                 break;
             }
        }

        if (!depth_sensor) {
            std::cerr << "Could not find depth sensor for applying options!" << std::endl;
            // Decide if this is fatal or not
        } else {
             std::cout << "\nApplying advanced camera options..." << std::endl;
             // Apply options using the helper function
             applyOption(depth_sensor, RS2_OPTION_LASER_POWER, config.getLaserPower(), "Laser Power");
             applyOption(depth_sensor, RS2_OPTION_CONFIDENCE_THRESHOLD, config.getConfidenceThreshold(), "Confidence Threshold");
             // applyOption(depth_sensor, RS2_OPTION_MIN_DISTANCE, config.getMinDistance(), "Min Distance"); // Read-Only
             applyOption(depth_sensor, RS2_OPTION_GAIN, config.getReceiverGain(), "Receiver Gain");
             applyOption(depth_sensor, RS2_OPTION_POST_PROCESSING_SHARPENING, config.getPostProcessingSharpening(), "Post Processing Sharpening");
             applyOption(depth_sensor, RS2_OPTION_NOISE_FILTERING, config.getNoiseFiltering(), "Noise Filtering");
             applyOption(depth_sensor, RS2_OPTION_INVALIDATION_BYPASS, config.getInvalidationBypass(), "Invalidation Bypass");
             // Error polling option not available in this SDK version
             // applyOption(depth_sensor, RS2_OPTION_ENABLE_ERROR_POLLING, config.getEnableErrorPolling(), "Error Polling Enabled");
             applyOption(depth_sensor, RS2_OPTION_INTER_CAM_SYNC_MODE, config.getInterCamSyncMode(), "Inter Cam Sync Mode");
             applyOption(depth_sensor, RS2_OPTION_FREEFALL_DETECTION_ENABLED, config.getFreefallDetectionEnabled(), "Freefall Detection Enabled");
             applyOption(depth_sensor, RS2_OPTION_EMITTER_ENABLED, config.getEmitterEnabled(), "Emitter Enabled");
             applyOption(depth_sensor, RS2_OPTION_VISUAL_PRESET, config.getVisualPreset(), "Visual Preset");
             applyOption(depth_sensor, RS2_OPTION_GLOBAL_TIME_ENABLED, config.getGlobalTimeEnabled(), "Global Time Enabled");

             // Add checks for other sensors (e.g., color sensor options) if needed
             std::cout << "Finished applying options.\n" << std::endl;
        }

        // Get depth scale (AFTER potentially setting options that might affect it, though unlikely)
        if (depth_sensor && depth_sensor.is<rs2::depth_sensor>()) {
            depth_scale = depth_sensor.as<rs2::depth_sensor>().get_depth_scale();
             std::cout << "Depth scale: " << depth_scale << " meters/unit" << std::endl;
        } else {
             std::cerr << "Could not get depth scale." << std::endl;
             // Handle error? Use a default?
        }

        // Wait for 1 second and capture frames (camera stabilization)
        for (int i = 0; i < config.getFps(); i++) {
            pipe.wait_for_frames();
        }
        
        return true;
    }
    catch (const rs2::error& e) {
        std::cerr << "RealSense initialization error: " << e.what() << " (" << e.get_failed_function() << ") - " << e.get_failed_args() << std::endl;
        // Stop pipeline if it was started before the error
         try { if(pipeline_started) pipe.stop(); } catch(...) {}
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
         try { if(pipeline_started) pipe.stop(); } catch(...) {}
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