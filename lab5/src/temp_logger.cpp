#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>  
#include <ctime>
#include <iomanip>
#include <chrono>  

#include "my_serial.hpp"

// Кроссплатформенный сон
#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(x) Sleep(x)
#else
    #include <unistd.h>
    #define SLEEP_MS(x) usleep((x)*1000)
#endif

using sys_clock = std::chrono::system_clock;

// --- Настройки симуляции ---
const int SIM_HOUR  = 5;   // 1 "час" = 5 секунд
const int SIM_DAY   = 10;  // 1 "сутки" = 10 секунд
const int SIM_MONTH = 20;  
const int SIM_YEAR  = 40; 

// Время хранения
const int RETENTION_RAW = SIM_DAY; 
const int RETENTION_HOUR = SIM_MONTH;
const int RETENTION_DAY = SIM_YEAR;

struct measure_t {
    sys_clock::time_point stamp;
    double val;
};

// Запись в файл
void save_data(const std::string& path, double val, sys_clock::time_point t, const std::string& tag) {
    std::time_t tt = sys_clock::to_time_t(t);
    std::tm tm = *std::localtime(&tt);
    
    std::ofstream file(path, std::ios::app);
    if (file.is_open()) {
        file << "TS: " << std::put_time(&tm, "%H:%M:%S") 
             << " | ID: " << std::left << std::setw(6) << tag 
             << " | VAL: " << std::fixed << std::setprecision(2) << val << "\n";
    }
}

// Ротация логов
void rotate_logs(const std::string& path, int max_age_seconds) {
    std::ifstream in(path);
    std::vector<std::string> keep_lines;
    std::string line;
    
    if (in.is_open()) {
        while (std::getline(in, line)) {
            if (!line.empty()) keep_lines.push_back(line);
        }
        in.close();
    }

    size_t limit = (max_age_seconds > 15) ? 50 : 20; 
    
    if (keep_lines.size() > limit) {
        std::ofstream out(path, std::ios::trunc);
        for (size_t i = keep_lines.size() - limit; i < keep_lines.size(); ++i) {
            out << keep_lines[i] << "\n";
        }
    }
}

// Чтение данных
std::vector<measure_t> read_recent_data(const std::string& path) {
    std::vector<measure_t> res;
    std::ifstream in(path);
    std::string line;
    
    if (in.is_open()) {
        while (std::getline(in, line)) {
            try {
                size_t p = line.find("VAL: ");
                if (p != std::string::npos) {
                    double v = std::stod(line.substr(p + 5));
                    res.push_back({sys_clock::now(), v});
                }
            } catch (...) {}
        }
    }
    return res;
}

void process_loop(cplib::SerialPort& port) {
    std::string buffer;
    auto t_hour = sys_clock::now();
    auto t_day = sys_clock::now();

    // Очистка старых логов при запуске
    std::remove("data_stream.log");
    std::remove("stat_hour.log");
    std::remove("stat_day.log");

    std::cout << "Waiting for data from device..." << std::endl;

    while (true) {
        std::string chunk;
        
        // Читаем кусочек данных
        int res = port.Read(chunk);
        
        // Если что-то пришло
        if (res == cplib::SerialPort::RE_OK && !chunk.empty()) {
            buffer += chunk;
            
            // Если буфер слишком разросся без \n - чистим, чтобы не лопнула память
            if (buffer.size() > 1024) buffer.clear();

            size_t p;
            while ((p = buffer.find('\n')) != std::string::npos) {
                std::string raw = buffer.substr(0, p);
                buffer.erase(0, p + 1);
                
                // Удаляем возможные символы возврата каретки \r (Windows)
                if (!raw.empty() && raw.back() == '\r') raw.pop_back();
                
                if (raw.empty()) continue;

                try {
                    double val = std::stod(raw);
                    auto now = sys_clock::now();
                    std::cout << "RX: " << val << " C" << std::endl;

                    save_data("data_stream.log", val, now, "RAW");
                    rotate_logs("data_stream.log", RETENTION_RAW);

                    // --- Логика "Часа" ---
                    auto diff_h = std::chrono::duration_cast<std::chrono::seconds>(now - t_hour).count();
                    if (diff_h >= SIM_HOUR) {
                        auto data = read_recent_data("data_stream.log");
                        if (!data.empty()) {
                            double sum = 0;
                            for(auto& m : data) sum += m.val;
                            double avg = sum / data.size();
                            std::cout << "[HOURLY] Avg: " << avg << std::endl;
                            save_data("stat_hour.log", avg, now, "HOUR");
                        }
                        rotate_logs("stat_hour.log", RETENTION_HOUR);
                        t_hour = now;
                    }

                    // --- Логика "Дня" ---
                    auto diff_d = std::chrono::duration_cast<std::chrono::seconds>(now - t_day).count();
                    if (diff_d >= SIM_DAY) {
                        auto data = read_recent_data("stat_hour.log");
                        if (!data.empty()) {
                            double sum = 0;
                            for(auto& m : data) sum += m.val;
                            double avg = sum / data.size();
                            std::cout << "[DAILY] Avg: " << avg << std::endl;
                            save_data("stat_day.log", avg, now, "DAY");
                        }
                        rotate_logs("stat_day.log", RETENTION_DAY);
                        t_day = now;
                    }

                } catch (...) {
                    // Игнорируем битые строки
                }
            }
        } else {
            // Данных нет - спим немного, чтобы не грузить ЦП
            SLEEP_MS(50);
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: LogTool <PORT>" << std::endl;
        return 1;
    }
    
    cplib::SerialPort port;
    std::cout << "Connecting to " << argv[1] << "..." << std::endl;
    
    if (port.Open(argv[1], cplib::SerialPort::BAUDRATE_9600) != cplib::SerialPort::RE_OK) {
        std::cerr << "Port error: " << argv[1] << " (Check if port exists or is used)" << std::endl;
        return 1;
    }
    
    port.Flush(); 
    port.SetTimeout(0.1);
    
    std::cout << "Logger started successfully." << std::endl;
    
    process_loop(port);
    
    return 0;
}