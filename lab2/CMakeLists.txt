CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(BackgroundRun)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(APP_SRCS src/test.cpp)
SET(LIB_SRCS src/background_run.cpp)
SET(LIB_HDRS include/background_run.hpp)


ADD_LIBRARY(background_lib SHARED ${LIB_SRCS} ${LIB_HDRS})

ADD_EXECUTABLE(test_process ${APP_SRCS})

TARGET_LINK_LIBRARIES(test_process background_lib)

