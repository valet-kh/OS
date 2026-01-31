#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>  
#include <ctime>
#include <iomanip>
#include <chrono>  
#include <algorithm>
#include <thread>

#include "my_serial.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::ofstream;
using std::ifstream;
using std::getline;
using std::exception;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::seconds;

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(x) Sleep(x)
#else
    #include <unistd.h>
    #define SLEEP_MS(x) usleep((x)*1000)
#endif

using sys_clock = system_clock;  

const int SIM_HOUR  = 5;   // 1 "час" = 5 секунд
const int SIM_DAY   = 10;  // 1 "сутки" = 10 секунд
const int SIM_MONTH = 20;  // 1 "месяц" = 20 секунд  
const int SIM_YEAR  = 40;  // 1 "год" = 40 секунд

const int RETENTION_RAW = SIM_DAY;      // 10 секунд (последние 24 часа) ~ 10 RAW записей в лог 
const int RETENTION_HOUR = SIM_MONTH;   // 20 секунд (последний месяц)  ~ 4 HOUR записи в лог 
const int RETENTION_DAY = SIM_YEAR;     // 40 секунд (весь год)         ~ 4 DAY записи в лог 

struct measure_t {
    sys_clock::time_point stamp;
    double val;
};

sys_clock::time_point parse_timestamp(const string& line) {
    auto now = sys_clock::now();
    
    try {
        size_t ts_pos = line.find("TS: ");
        if (ts_pos != string::npos) {
            string time_str = line.substr(ts_pos + 4, 8);
            
            int hours = stoi(time_str.substr(0, 2));   
            int minutes = stoi(time_str.substr(3, 2)); 
            int seconds = stoi(time_str.substr(6, 2));
            
            time_t now_t = sys_clock::to_time_t(now);
            struct tm* tm_info = localtime(&now_t);
            
            tm_info->tm_hour = hours;
            tm_info->tm_min = minutes;
            tm_info->tm_sec = seconds;
            
            return sys_clock::from_time_t(mktime(tm_info));
        }
    } catch (const exception& e) {
        cerr << "Error parsing timestamp: " << e.what() << endl;
    }
    
    return now;
}

void save_data(const string& path, double val, sys_clock::time_point t, const string& tag) {
    time_t tt = sys_clock::to_time_t(t);
    tm tm = *localtime(&tt);
    
    ofstream file(path, ofstream::app); 
    if (file.is_open()) {
        file << "TS: " << std::put_time(&tm, "%H:%M:%S") 
             << " | TYPE: " << std::left << std::setw(6) << tag 
             << " | VAL: " << std::fixed << std::setprecision(2) << val << "\n";
        file.close();
    }
}

void rotate_logs_by_time(const string& path, int max_age_seconds) {
    ifstream in(path);
    vector<string> keep_lines;
    string line;
    
    auto now = sys_clock::now();
    
    if (in.is_open()) {
        while (getline(in, line)) {
            if (line.empty()) continue;
            
            auto line_time = parse_timestamp(line);
            auto age = duration_cast<seconds>(now - line_time).count();
            
            if (age <= max_age_seconds) {
                keep_lines.push_back(line);
            }
        }
        in.close();
    }
    
    ofstream out(path, ofstream::trunc);
    for (const auto& l : keep_lines) {
        out << l << "\n";
    }
}

vector<measure_t> read_recent_data(const string& path, int max_age_seconds = SIM_HOUR) {
    vector<measure_t> res;
    ifstream in(path);
    string line;
    
    auto now = sys_clock::now();
    
    if (in.is_open()) {
        while (getline(in, line)) {
            try {
                auto line_time = parse_timestamp(line);
                auto age = duration_cast<seconds>(now - line_time).count();
                
                if (age <= max_age_seconds) {
                    size_t p = line.find("VAL: ");
                    if (p != string::npos) {
                        double v = stod(line.substr(p + 5));
                        res.push_back({line_time, v});
                    }
                }
            } catch (...) {}
        }
    }
    return res;
}

void process_loop(cplib::SerialPort& port) {
    string line_buffer;
    auto t_hour = sys_clock::now();
    auto t_day = sys_clock::now();

    remove("data_stream.log");
    remove("stat_hour.log");
    remove("stat_day.log");

    cout << "Start process_loop" << endl;
    cout << std::fixed << std::setprecision(2);


    while (true) {
        char read_buf[256];
        size_t bytes_read = 0;
        
        int res = port.Read(read_buf, sizeof(read_buf) - 1, &bytes_read);
        
        if (res == cplib::SerialPort::RE_OK && bytes_read > 0) {
            read_buf[bytes_read] = '\0';
            line_buffer += read_buf;
            
            size_t p;
            while ((p = line_buffer.find('\n')) != string::npos) {
                string raw = line_buffer.substr(0, p);
                line_buffer.erase(0, p + 1);
                
                if (raw.empty()) continue;

                try {
                    double val = stod(raw);
                    auto now = sys_clock::now();
                    cout << "RX: " << val << " C" << endl;

                    // сохранение RAW данных
                    save_data("data_stream.log", val, now, "RAW");
                    rotate_logs_by_time("data_stream.log", RETENTION_RAW);

                    // проверка на часовой интервал и сохранение HOUR данных
                    auto diff_h = duration_cast<seconds>(now - t_hour).count();
                    if (diff_h >= SIM_HOUR) {
                        auto data = read_recent_data("data_stream.log", SIM_HOUR);
                        if (!data.empty()) {
                            double sum = 0;
                            for(auto& m : data) sum += m.val;
                            double avg = sum / data.size();
                            cout << "[HOURLY] Avg: " << avg << endl;
                            save_data("stat_hour.log", avg, now, "HOUR");
                        }
                        rotate_logs_by_time("stat_hour.log", RETENTION_HOUR);
                        t_hour = now;
                    }

                    // проверка на дневной интервал и сохранение DAY данных
                    auto diff_d = duration_cast<seconds>(now - t_day).count();
                    if (diff_d >= SIM_DAY) {
                        auto data = read_recent_data("stat_hour.log", SIM_DAY);
                        if (!data.empty()) {
                            double sum = 0;
                            for(auto& m : data) sum += m.val;
                            double avg = sum / data.size();
                            cout << "[DAILY] Avg: " << avg << endl;
                            save_data("stat_day.log", avg, now, "DAY");
                        }
                        rotate_logs_by_time("stat_day.log", RETENTION_DAY);
                        t_day = now;
                    }

                } catch (const exception& e) {
                    cerr << "Parse error: " << e.what() << " on raw: " << raw << endl;
                }
            }
        } else {
            SLEEP_MS(50);
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: LogTool <PORT>" << endl;
        cout << "Example: LogTool COM (Windows) or LogTool /dev/ttyUSB0 (Linux)" << endl;
        return 1;
    }
    
    cplib::SerialPort port;
    cout << "Connecting to " << argv[1] << "..." << endl;
    
    cplib::SerialPort::Parameters params(cplib::SerialPort::BAUDRATE_9600);
    
    if (port.Open(argv[1], params) != cplib::SerialPort::RE_OK) {
        cerr << "Port error: " << argv[1] << endl;
        return 1;
    }
    
    port.Flush(); 
    port.SetTimeout(0.1); 
    
    cout << "Logger start" << endl;
    
    try {
        process_loop(port);
    } catch (const exception& e) {
        cerr << "Error in process_loop: " << e.what() << endl;
    }
    
    port.Close();
    return 0;
}