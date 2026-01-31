#include "logger.hpp"
#include <cstdarg>

Logger::Logger(const char* filename) {
    file = std::fopen(filename, "a");
}

Logger::~Logger() {
    if (file) std::fclose(file);
}

bool Logger::is_valid() const {
    return file != nullptr;
}

void Logger::write(const char* format, ...) {
    if (!file) return;
    
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    char time_buffer[80];
    std::strftime(time_buffer, sizeof(time_buffer), 
                  "%Y-%m-%d %H:%M:%S", 
                  std::localtime(&now_time_t));
    
    std::fprintf(file, "%s.%03lld - ", time_buffer, (long long)now_ms.count());
    
    va_list args;
    va_start(args, format);
    std::vfprintf(file, format, args);
    va_end(args);
    
    std::fprintf(file, "\n");
    std::fflush(file);
}