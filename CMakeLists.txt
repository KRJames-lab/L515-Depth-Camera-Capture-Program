cmake_minimum_required(VERSION 3.10)
project(depth_capture)

# C++ standard settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenMP settings
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# RealSense library settings
find_package(realsense2 REQUIRED)

# OpenCV library settings
find_package(OpenCV REQUIRED)

# YAML-CPP library settings
find_package(yaml-cpp REQUIRED)

# Source files list
set(SOURCES
    main.cpp
    load_config.cpp
    camera.cpp
    processor.cpp
)

# Header files list
set(HEADERS
    load_config.h
    camera.h
    processor.h
)

# Generate executable
add_executable(depth_capture ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(depth_capture 
    ${realsense2_LIBRARY}
    ${OpenCV_LIBS}
    yaml-cpp
    ${CMAKE_THREAD_LIBS_INIT}
)

# Header directories settings
target_include_directories(depth_capture PUBLIC
    ${realsense2_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIR}
)

# Installation target settings
install(TARGETS depth_capture DESTINATION bin)

# Default configuration file installation
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/depth_config.yaml 
               ${CMAKE_CURRENT_BINARY_DIR}/depth_config.yaml COPYONLY)

# Build type settings (Release mode default)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Release mode optimization settings
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Build information output
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "OpenMP Flags: ${OpenMP_CXX_FLAGS}") 