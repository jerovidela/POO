cmake_minimum_required(VERSION 3.15)
project(tp2_file233 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tp2
    src/main.cpp
    src/File_233.cpp
    src/SerialReader.cpp
)
target_include_directories(tp2 PRIVATE include)