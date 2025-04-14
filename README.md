# L515 Depth Camera Capture Program

This program is a C++ application that captures and saves depth maps and color images using the Intel RealSense L515 LiDAR camera.

## Features

- Depth map and color image capture
- Support for various output formats (binary, PNG images)
- YAML-based configuration file
- Parallel processing optimization using OpenMP
- Direct memory access option
- Infrared and confidence stream support (L515 exclusive)

## System Requirements

- C++14 compatible compiler or higher
- CMake 3.10 or higher
- librealsense2 SDK (version 2.50.0 or higher recommended)
- OpenCV library (version 4.0 or higher recommended)
- YAML-CPP library
- OpenMP compatible compiler

## Build Instructions

```bash
# 1. Clone the repository
git clone https://github.com/username/l515-depth-capture.git
cd l515-depth-capture

# 2. Create a build directory
mkdir build && cd build

# 3. Configure and build with CMake
cmake ..
make

# 4. (Optional) Install
sudo make install
```

## Usage

```bash
# Run with default settings
./depth_capture

# Results are saved in the result directory:
# - depth_data.bin (depth map binary data)
# - color_image.png (color image, if enabled)
# - infrared_image.png (infrared image, if enabled)
```

## Configuration File (depth_config.yaml)

The program loads settings from the `depth_config.yaml` file. Main configuration items:

```yaml
# Example settings
depth_width: 1024    # Depth map resolution (width)
depth_height: 768    # Depth map resolution (height)
fps: 30              # Frames per second
save_depth_binary: true  # Save depth map as binary
num_threads: 8       # Number of OpenMP threads to use
```

All configuration options are detailed in the `depth_config.yaml` file.

## Code Structure

- **load_config.h/cpp**: Configuration related classes and functions
- **camera.h/cpp**: RealSense camera related classes and functions
- **processor.h/cpp**: Depth data processing and storage related classes
- **main.cpp**: Main program logic

## Notes

- Ensure the RealSense L515 camera is connected before running the program.
- Other RealSense models will also work, but some special features (e.g., confidence stream) are exclusive to the L515.
- For optimal performance, connect the camera to a USB 3.0/3.1 port.

## License

This project is distributed under the MIT License. See the LICENSE file for details. 