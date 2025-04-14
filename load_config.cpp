#include "load_config.h"

// Initialize static member
std::map<std::string, rs2_format> LoadConfig::format_map = {
    {"Z16", RS2_FORMAT_Z16},
    {"Y8", RS2_FORMAT_Y8},
    {"Y16", RS2_FORMAT_Y16},
    {"RGB8", RS2_FORMAT_RGB8},
    {"BGR8", RS2_FORMAT_BGR8},
    {"RGBA8", RS2_FORMAT_RGBA8},
    {"BGRA8", RS2_FORMAT_BGRA8},
    {"RAW8", RS2_FORMAT_RAW8},
    {"YUYV", RS2_FORMAT_YUYV}
};

// Default constructor
LoadConfig::LoadConfig() {
    // Default settings are already set in member initialization list
}

// Load from configuration file
bool LoadConfig::loadFromFile(const std::string& filename) {
    try {
        YAML::Node yaml = YAML::LoadFile(filename);
        
        // Camera settings
        if (yaml["depth_width"]) depth_width = yaml["depth_width"].as<int>();
        if (yaml["depth_height"]) depth_height = yaml["depth_height"].as<int>();
        if (yaml["color_width"]) color_width = yaml["color_width"].as<int>();
        if (yaml["color_height"]) color_height = yaml["color_height"].as<int>();
        if (yaml["fps"]) fps = yaml["fps"].as<int>();
        if (yaml["output_dir"]) output_dir = yaml["output_dir"].as<std::string>();
        
        // Format settings
        if (yaml["depth_format"]) {
            std::string format = yaml["depth_format"].as<std::string>();
            if (isValidFormat(format, "depth")) {
                depth_format = format;
            } else {
                std::cerr << "Warning: Unsupported depth format. Using default (Z16)." << std::endl;
            }
        }
        
        if (yaml["color_format"]) {
            std::string format = yaml["color_format"].as<std::string>();
            if (isValidFormat(format, "color")) {
                color_format = format;
            } else {
                std::cerr << "Warning: Unsupported color format. Using default (RGB8)." << std::endl;
            }
        }
        
        // Additional stream enable options
        if (yaml["enable_infrared"]) enable_infrared = yaml["enable_infrared"].as<bool>();
        if (yaml["enable_confidence"]) enable_confidence = yaml["enable_confidence"].as<bool>();
        
        // Data format settings
        if (yaml["float_precision"]) {
            int precision = yaml["float_precision"].as<int>();
            if (precision == 32 || precision == 64) {
                float_precision = precision;
            } else {
                std::cerr << "Floating point precision must be 32 or 64. Using default (32-bit)." << std::endl;
            }
        }
        
        // Performance settings
        if (yaml["num_threads"]) num_threads = yaml["num_threads"].as<int>();
        if (yaml["use_direct_access"]) use_direct_access = yaml["use_direct_access"].as<bool>();
        
        // Output settings
        if (yaml["save_color_image"]) save_color_image = yaml["save_color_image"].as<bool>();
        if (yaml["save_depth_map"]) save_depth_map = yaml["save_depth_map"].as<bool>();
        if (yaml["save_depth_raw"]) save_depth_raw = yaml["save_depth_raw"].as<bool>();
        if (yaml["save_depth_binary"]) save_depth_binary = yaml["save_depth_binary"].as<bool>();
        if (yaml["save_infrared"]) save_infrared = yaml["save_infrared"].as<bool>();
        if (yaml["csv_sampling_step"]) csv_sampling_step = yaml["csv_sampling_step"].as<int>();
        if (yaml["binary_filename"]) binary_filename = yaml["binary_filename"].as<std::string>();
        
        std::cout << "YAML configuration loaded: " << filename << std::endl;
        return true;
    }
    catch (const YAML::Exception& e) {
        std::cerr << "YAML configuration file load error: " << e.what() << std::endl;
        std::cout << "Using default settings." << std::endl;
        return false;
    }
}

// rs2_format conversion
rs2_format LoadConfig::getDepthRs2Format() const {
    return format_map[depth_format];
}

rs2_format LoadConfig::getColorRs2Format() const {
    return format_map[color_format];
}

// Stream format validation
bool LoadConfig::isValidFormat(const std::string& format, const std::string& stream_type) {
    if (format_map.find(format) == format_map.end()) {
        return false;
    }
    
    // Check allowed formats for each stream type
    if (stream_type == "depth" && format != "Z16") {
        return false; // Depth only supports Z16
    }
    
    if (stream_type == "infrared" && format != "Y8") {
        return false; // Infrared only supports Y8
    }
    
    if (stream_type == "confidence" && format != "RAW8") {
        return false; // Confidence only supports RAW8
    }
    
    return true;
} 