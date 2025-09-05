# project
cmake_minimum_required(VERSION 3.20.0)
project(audio-capturex LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib")

# Enable CPM
include(cmake/cpm-downloader.cmake)

# Include vendor CMakeLists.txt files
add_subdirectory(vendor/cubeb)
add_subdirectory(vendor/drwav)

# Create library
add_library(audio-capturex STATIC
    src/audio_capture.cpp
)

# Include directories
target_include_directories(audio-capturex PUBLIC include)
target_include_directories(audio-capturex PRIVATE ${cubeb_SOURCE_DIR}/include)
target_include_directories(audio-capturex PRIVATE ${CMAKE_BINARY_DIR}/exports)

# Link libraries
target_link_libraries(audio-capturex PRIVATE cubeb drwav)

# Create executable
add_executable(sample src/main.cpp)

# Link library to executable
target_link_libraries(sample PRIVATE audio-capturex)
target_include_directories(sample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(sample PRIVATE ${cubeb_SOURCE_DIR}/include)
target_include_directories(sample PRIVATE ${CMAKE_BINARY_DIR}/exports)
