cmake_minimum_required(VERSION 3.10)
project(DICI)

# Forcer l'utilisation de C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Définir explicitement le répertoire des bibliothèques OpenCV statiques
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib")

# Spécifier que nous voulons utiliser les bibliothèques statiques d'OpenCV
set(OpenCV_STATIC ON)

# Trouver OpenCV
find_package(OpenCV REQUIRED)

# Inclure les répertoires des headers
include_directories(${OpenCV_INCLUDE_DIRS} includes lib)

# DBUG
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")
message(STATUS "OpenCV_DIR: ${OpenCV_DIR}")

# Ajouter explicitement les répertoires des bibliothèques statiques
link_directories(C:/opencv/build/x64/vc16/lib lib)

# ==========================
# 🔵 Compilation de DICI-encode
# ==========================
add_executable(DICI-encode 
    DICI-encode.cpp 
    sources/DICIencode.cpp 
    sources/Encode.cpp 
    sources/filter.cpp)

target_link_libraries(DICI-encode ${OpenCV_LIBS} lzma)

set_target_properties(DICI-encode PROPERTIES OUTPUT_NAME "DICI-encode")

# ==========================
# 🟢 Compilation de DICI-decode
# ==========================
add_executable(DICI-decode 
    DICI-decode.cpp 
    sources/DICIdecode.cpp 
    sources/Decode.cpp 
    sources/filter_reverse.cpp)

target_link_libraries(DICI-decode ${OpenCV_LIBS} lzma)

set_target_properties(DICI-decode PROPERTIES OUTPUT_NAME "DICI-decode")
