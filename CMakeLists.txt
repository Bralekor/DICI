cmake_minimum_required(VERSION 3.10)
project(DICI)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

file(GLOB LZMA_SDK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/includes/lzma/*.c")

include_directories(${OpenCV_INCLUDE_DIRS} includes ${CMAKE_CURRENT_SOURCE_DIR}/includes/lzma)

add_executable(DICI-encode
    DICI-encode.cpp
    sources/DICIencode.cpp
    sources/encode.cpp
    sources/filter.cpp
    ${LZMA_SDK_SOURCES} # <-- Ajout des sources du LZMA SDK
)


target_link_libraries(DICI-encode ${OpenCV_LIBS})

add_executable(DICI-decode
    DICI-decode.cpp
    sources/DICIdecode.cpp
    sources/decode.cpp
    sources/filter_reverse.cpp
    ${LZMA_SDK_SOURCES} # <-- Ajout des sources du LZMA SDK
)

target_link_libraries(DICI-decode ${OpenCV_LIBS})