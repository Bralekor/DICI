cmake_minimum_required(VERSION 3.10)
project(DICI)

# Force the use of C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Explicitly set the directory for static OpenCV libraries
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib")

# Specify that we want to use static OpenCV libraries
set(OpenCV_STATIC ON)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include header directories
include_directories(${OpenCV_INCLUDE_DIRS} includes)

# Debug information
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")
message(STATUS "OpenCV_DIR: ${OpenCV_DIR}")

# Add explicit directories for static libraries
link_directories(C:/opencv/build/x64/vc16/lib lib)

# ==========================
# 🔵 Compilation of DICI-encode
# ==========================
add_executable(DICI-encode 
    DICI-encode.cpp 
    sources/DICIencode.cpp 
    sources/Encode.cpp 
    sources/filter.cpp)

# Link OpenCV and LZMA libraries
target_link_libraries(DICI-encode ${OpenCV_LIBS} lzma)

# Set the output name for DICI-encode
set_target_properties(DICI-encode PROPERTIES OUTPUT_NAME "DICI-encode")

# ==========================
# 🟢 Compilation of DICI-decode
# ==========================
add_executable(DICI-decode 
    DICI-decode.cpp 
    sources/DICIdecode.cpp 
    sources/Decode.cpp 
    sources/filter_reverse.cpp)

# Link OpenCV and LZMA libraries
target_link_libraries(DICI-decode ${OpenCV_LIBS} lzma)

# Set the output name for DICI-decode
set_target_properties(DICI-decode PROPERTIES OUTPUT_NAME "DICI-decode")
