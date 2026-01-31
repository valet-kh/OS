#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cstdio>
#include <chrono>
#include <cstdarg>
#include <ctime>

class Logger {
private:
    FILE* file;
    
public:
    Logger(const char* filename);
    ~Logger();
    bool is_valid() const;
    void write(const char* format, ...);
};

#endif