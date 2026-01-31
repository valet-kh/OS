cmake_minimum_required(VERSION 3.10)
project(SerialPortLab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(Threads REQUIRED)

add_executable(DeviceSim src/sensor_emitter.cpp)

add_executable(LogTool src/temp_logger.cpp)

if(NOT WIN32)
    target_link_libraries(DeviceSim PRIVATE Threads::Threads rt)
    target_link_libraries(LogTool PRIVATE Threads::Threads rt)
else()
    target_link_libraries(DeviceSim PRIVATE Threads::Threads)
    target_link_libraries(LogTool PRIVATE Threads::Threads)
endif()