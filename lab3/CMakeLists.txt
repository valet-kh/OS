CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(lab3)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)


SET(MAIN_SRCS 
    src/main.cpp
    src/program.cpp
    src/counter.cpp
    src/process.cpp
    src/logger.cpp
)

ADD_EXECUTABLE(lab3 ${MAIN_SRCS})


IF(NOT WIN32)
    TARGET_LINK_LIBRARIES(lab3 rt)
ENDIF(NOT WIN32)