cmake_minimum_required(VERSION 3.12)
project(HighSpeedDetector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(HighSpeedDetector main.cpp)
target_link_libraries(HighSpeedDetector PRIVATE SDL2::SDL2 ${OpenCV_LIBS})