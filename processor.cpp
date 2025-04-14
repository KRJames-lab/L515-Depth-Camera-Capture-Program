#include "processor.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sys/stat.h>

// Function to check if directory exists
bool dirExists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

// Function to create directory
bool createDir(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0;
}

// Constructor
Processor::Processor(const LoadConfig& config, const Camera& camera)
    : config(config), camera(camera) {
    // Create result directory
    createResultDirectory();
}

// Create result directory
void Processor::createResultDirectory() {
    std::string dir = config.getOutputDir();
    
    // Add separator at the end of path (if not present)
    if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') {
        dir += '/';
    }
    
    // Create directory
    try {
        if (!dirExists(dir)) {
            if (createDir(dir)) {
                std::cout << "Result directory created: " << dir << std::endl;
            } else {
                std::cerr << "Failed to create result directory: " << dir << std::endl;
            }
        } else {
            std::cout << "Result directory already exists: " << dir << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing result directory: " << e.what() << std::endl;
    }
    
    // Save directory path
    output_dir = dir;
}

// Process depth frame
bool Processor::processDepthFrame() {
    try {
        // Get depth frame from camera
        auto depth_frame = camera.getDepthFrame();
        if (!depth_frame) {
            std::cerr << "Depth frame is not valid.\n";
            return false;
        }
        
        // Get actual frame dimensions
        int width = depth_frame.as<rs2::video_frame>().get_width();
        int height = depth_frame.as<rs2::video_frame>().get_height();
        
        // Create raw depth data only if needed
        if (config.saveDepthRaw()) {
            depth_raw = cv::Mat(cv::Size(width, height), 
                               CV_16U, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);
        }
        
        // Select appropriate data type based on configured precision
        int depth_type = (config.getFloatPrecision() == 64) ? CV_64F : CV_32F;
        
        // Create matrix for depth data in meters
        depth_meters = cv::Mat(cv::Size(width, height), depth_type);
        
        // Start performance measurement
        auto start = std::chrono::high_resolution_clock::now();
        
        // Get depth scale
        float depth_scale = camera.getDepthScale();
        
        // Convert each pixel's depth to meters - accelerated with OpenMP
        if (config.useDirectAccess()) {
            // Direct memory access method (fast)
            const uint16_t* raw_data = reinterpret_cast<const uint16_t*>(depth_frame.get_data());
            
            if (depth_meters.isContinuous()) {
                // Continuous memory - 1D parallelization
                if (depth_type == CV_32F) {
                    float* depth_ptr = reinterpret_cast<float*>(depth_meters.data);
                    size_t total = width * height;
                    
                    #pragma omp parallel for schedule(static)
                    for (int i = 0; i < total; i++) {
                        depth_ptr[i] = raw_data[i] * depth_scale;
                    }
                } else {
                    double* depth_ptr = reinterpret_cast<double*>(depth_meters.data);
                    size_t total = width * height;
                    
                    #pragma omp parallel for schedule(static)
                    for (int i = 0; i < total; i++) {
                        depth_ptr[i] = raw_data[i] * depth_scale;
                    }
                }
            } else {
                // Non-continuous memory processing in parallel
                #pragma omp parallel for collapse(2) schedule(static)
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int idx = y * width + x;
                        if (depth_type == CV_32F) {
                            depth_meters.at<float>(y, x) = raw_data[idx] * depth_scale;
                        } else {
                            depth_meters.at<double>(y, x) = raw_data[idx] * depth_scale;
                        }
                    }
                }
            }
        } else {
            // OpenCV at() access method (safer but relatively slower)
            #pragma omp parallel for collapse(2) schedule(static)
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    uint16_t depth_value = ((uint16_t*)depth_frame.get_data())[y * width + x];
                    
                    if (depth_type == CV_64F) {
                        depth_meters.at<double>(y, x) = static_cast<double>(depth_value) * depth_scale;
                    } else {
                        depth_meters.at<float>(y, x) = static_cast<float>(depth_value) * depth_scale;
                    }
                }
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Depth conversion processing time: " << duration.count() << "ms" << std::endl;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing depth frame: " << e.what() << std::endl;
        return false;
    }
}

// Process color frame
bool Processor::processColorFrame() {
    try {
        if (!config.saveColorImage()) {
            return true; // Success if color image not needed
        }
        
        // Get color frame from camera
        auto color_frame = camera.getColorFrame();
        if (!color_frame) {
            std::cerr << "Color frame is not valid.\n";
            return false;
        }
        
        // Process color image
        color_image = cv::Mat(cv::Size(color_frame.as<rs2::video_frame>().get_width(), 
                                      color_frame.as<rs2::video_frame>().get_height()), 
                            CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);
                            
        // Convert RGB/BGR if needed
        if (config.getColorFormat() == "RGB8") {
            cv::cvtColor(color_image, color_image, cv::COLOR_RGB2BGR);
        }
        
        // Save image
        cv::imwrite(output_dir + "color_image.png", color_image);
        std::cout << "Color image saved: " << output_dir << "color_image.png" << std::endl;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing color frame: " << e.what() << std::endl;
        return false;
    }
}

// Process infrared frame
bool Processor::processInfraredFrame() {
    try {
        if (!config.enableInfrared() || !config.saveInfrared()) {
            return true; // Success if infrared image not needed
        }
        
        // Get infrared frame from camera
        auto ir_frame = camera.getInfraredFrame();
        if (!ir_frame) {
            std::cerr << "Infrared frame is not valid.\n";
            return false;
        }
        
        // Process infrared image
        ir_image = cv::Mat(cv::Size(ir_frame.as<rs2::video_frame>().get_width(), 
                                   ir_frame.as<rs2::video_frame>().get_height()), 
                          CV_8UC1, (void*)ir_frame.get_data(), cv::Mat::AUTO_STEP);
        
        // Save image
        cv::imwrite(output_dir + "infrared_image.png", ir_image);
        std::cout << "Infrared image saved: " << output_dir << "infrared_image.png" << std::endl;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing infrared frame: " << e.what() << std::endl;
        return false;
    }
}

// Save depth data
bool Processor::saveDepthData() {
    try {
        // Save binary file
        if (config.saveDepthBinary()) {
            std::string binary_path = output_dir + config.getBinaryFilename();
            
            if (config.getFloatPrecision() == 64) {
                saveDepthBinary<double>(depth_meters, binary_path);
            } else {
                saveDepthBinary<float>(depth_meters, binary_path);
            }
        }
        
        // Save depth map as image
        if (config.saveDepthMap()) {
            // Normalize depth data for visualization (0-255)
            cv::Mat depth_normalized;
            double min_val = 0, max_val = 0;
            
            // Find min/max values for normalization
            cv::minMaxLoc(depth_meters, &min_val, &max_val);
            
            // Convert to 8-bit for visualization
            depth_meters.convertTo(depth_normalized, CV_8UC1, 255.0 / (max_val - min_val), -min_val * 255.0 / (max_val - min_val));
            
            // Apply colormap for better visualization
            cv::Mat depth_colormap;
            cv::applyColorMap(depth_normalized, depth_colormap, cv::COLORMAP_JET);
            
            // Save the image
            std::string image_path = output_dir + "depth_map.png";
            cv::imwrite(image_path, depth_colormap);
            std::cout << "Depth map image saved: " << image_path << std::endl;
        }
        
        // Print depth map info
        printDepthInfo();
        
        // Save depth data to CSV
        if (config.saveDepthCsv()) {
            std::string csv_path = output_dir + "depth_data.csv";
            std::ofstream csv_file(csv_path);
            
            if (!csv_file.is_open()) {
                std::cerr << "Cannot open CSV file: " << csv_path << std::endl;
                return false;
            }
            
            // Get sampling step to reduce file size
            int step = config.getCsvSamplingStep();
            if (step < 1) step = 1;
            
            // Write CSV header
            csv_file << "x,y,depth_m" << std::endl;
            
            // Write depth data
            for (int y = 0; y < depth_meters.rows; y += step) {
                for (int x = 0; x < depth_meters.cols; x += step) {
                    float depth_value;
                    if (depth_meters.type() == CV_32F) {
                        depth_value = depth_meters.at<float>(y, x);
                    } else {
                        depth_value = (float)depth_meters.at<double>(y, x);
                    }
                    
                    // 모든 픽셀 저장 (조건문 제거)
                    csv_file << x << "," << y << "," << depth_value << std::endl;
                }
            }
            
            csv_file.close();
            std::cout << "Depth data CSV saved: " << csv_path << std::endl;
            std::cout << "  - Sampling step: " << step << std::endl;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving depth data: " << e.what() << std::endl;
        return false;
    }
}

// Save depth data to binary file - OpenMP optimized version
template <typename T>
bool Processor::saveDepthBinary(const cv::Mat& depth_data, const std::string& filename) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Prepare memory
    int rows = depth_data.rows;
    int cols = depth_data.cols;
    int type = depth_data.type();
    size_t header_size = 3 * sizeof(int);
    size_t data_size = depth_data.total() * depth_data.elemSize();
    size_t total_size = header_size + data_size;
    
    // Create memory buffer (for writing at once)
    std::vector<char> buffer(total_size);
    
    // Copy header information
    memcpy(buffer.data(), &rows, sizeof(int));
    memcpy(buffer.data() + sizeof(int), &cols, sizeof(int));
    memcpy(buffer.data() + 2 * sizeof(int), &type, sizeof(int));
    
    // Copy data
    if (depth_data.isContinuous()) {
        // Continuous memory can be copied at once with memcpy
        memcpy(buffer.data() + header_size, depth_data.data, data_size);
    } else {
        // Non-continuous memory processed with regular for loop
        for (int y = 0; y < rows; y++) {
            memcpy(buffer.data() + header_size + y * cols * depth_data.elemSize(), 
                  depth_data.ptr(y), cols * depth_data.elemSize());
        }
    }
    
    // Save to file
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open binary file: " << filename << std::endl;
        return false;
    }
    
    // Increase file buffer size (64KB)
    char file_buffer[65536];
    file.rdbuf()->pubsetbuf(file_buffer, sizeof(file_buffer));
    
    // Write at once
    file.write(buffer.data(), total_size);
    file.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Binary depth data saved: " << filename << std::endl;
    std::cout << "  - Size: " << cols << "x" << rows << " pixels" << std::endl;
    std::cout << "  - File size: " << total_size / (1024.0 * 1024.0) << "MB" << std::endl;
    std::cout << "  - Save time: " << duration.count() << "ms" << std::endl;
    
    return true;
}

// Print depth map information
void Processor::printDepthInfo() const {
    int width = depth_meters.cols;
    int height = depth_meters.rows;
    int original_size = width * height;
    
    std::cout << "\nDepth map information:" << std::endl;
    std::cout << "  - Resolution: " << width << "x" << height 
              << " (" << original_size << " pixels)" << std::endl;
    std::cout << "  - Depth format: " << config.getDepthFormat() << std::endl;
    std::cout << "  - Floating point precision: " << config.getFloatPrecision() << " bits" << std::endl;
    std::cout << "  - Data size: " << (original_size * (config.getFloatPrecision() / 8)) / (1024 * 1024.0) 
              << "MB" << std::endl;
}

// Template instantiation (prevent linker errors)
template bool Processor::saveDepthBinary<float>(const cv::Mat&, const std::string&);
template bool Processor::saveDepthBinary<double>(const cv::Mat&, const std::string&); 