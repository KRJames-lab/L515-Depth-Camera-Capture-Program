#include <iostream>
#include <omp.h>
#include "load_config.h"
#include "camera.h"
#include "processor.h"

// Configuration file path
const std::string CONFIG_FILE = "depth_config.yaml";

int main() {
    try {
        // Load configuration
        LoadConfig config;
        if (!config.loadFromFile(CONFIG_FILE)) {
            std::cout << "Using default settings." << std::endl;
        }
        
        // OpenMP thread settings
        if (config.getNumThreads() > 0) {
            omp_set_num_threads(config.getNumThreads());
        }
        
        std::cout << "OpenMP max threads: " << omp_get_max_threads() << std::endl;
        
        // Initialize camera
        Camera camera(config);
        if (!camera.initialize()) {
            std::cerr << "Camera initialization failed\n";
            return 1;
        }
        
        // Capture frames
        if (!camera.captureFrames()) {
            std::cerr << "Frame capture failed\n";
            return 1;
        }
        
        // Data processing
        Processor processor(config, camera);
        
        // Process depth frame
        if (!processor.processDepthFrame()) {
            std::cerr << "Depth frame processing failed\n";
            return 1;
        }
        
        // Process color image
        processor.processColorFrame();
        
        // Process infrared image
        processor.processInfraredFrame();
        
        // Save depth data
        if (!processor.saveDepthData()) {
            std::cerr << "Depth data save failed\n";
            return 1;
        }
        
        std::cout << "Program completed successfully\n";
    }
    catch (const rs2::error& e) {
        std::cerr << "RealSense error: " << e.what() << " (" << e.get_failed_function() << ")" << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
} 