#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <iomanip>

#include "http_server.hpp"
#include "my_serial.hpp"
#include "database.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

using std::chrono::duration_cast;
using std::chrono::seconds;


const int SIM_HOUR  = 5;   // 1 "час" = 5 секунд
const int SIM_DAY   = 10;  // 1 "сутки" = 10 секунд
const int SIM_MONTH = 20;  // 1 "месяц" = 20 секунд  
const int SIM_YEAR  = 40;  // 1 "год" = 40 секунд

const int RETENTION_RAW = SIM_DAY;      // 10 секунд (последние 24 часа) ~ 10 RAW записей в лог 
const int RETENTION_HOUR = SIM_MONTH;   // 20 секунд (последний месяц)  ~ 4 HOUR записи в лог 
const int RETENTION_DAY = SIM_YEAR;     // 40 секунд (весь год)         ~ 4 DAY записи в лог 


using sys_clock = std::chrono::system_clock;

DBManager db;
std::mutex db_mutex; 

string records_to_json(const vector<DBRecord>& records) {
    string json = "[";
    for (size_t i = 0; i < records.size(); ++i) {
        json += "{\"t\":" + std::to_string(records[i].timestamp) + ", \"y\":" + std::to_string(records[i].value) + "}";
        if (i < records.size() - 1) json += ",";
    }
    json += "]";
    return json;
}


void data_collection_loop(cplib::SerialPort& port) {
    string line_buffer;  // ИЗМЕНЕНО: buffer -> line_buffer для ясности
    auto last_hour_calc = sys_clock::now();
    auto last_day_calc = sys_clock::now();

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
                
                if (!raw.empty() && raw.back() == '\r') raw.pop_back();

                if (raw.empty()) continue;  

                try {
                    double val = std::stod(raw);
                    auto now = sys_clock::now();
                    long long ts = duration_cast<seconds>(now.time_since_epoch()).count();

                    // сохранение raw_data
                    cout << "[SENSOR] Value: " << std::fixed << std::setprecision(2) << val << endl;
                    {
                        std::lock_guard<std::mutex> lock(db_mutex);
                        db.Insert("raw_data", ts, val);
                        db.CleanupOldData("raw_data", ts - RETENTION_RAW);
                    }

                    // проверка на часовой интервал и сохранение hourly_stats
                    auto diff_h = duration_cast<seconds>(now - last_hour_calc).count();
                    if (diff_h >= SIM_HOUR) {
                        std::lock_guard<std::mutex> lock(db_mutex);
                        double avg = db.GetAverage("raw_data", ts - SIM_HOUR);
                        if (avg > 0.001) { 
                            cout << "[HOURLY] Avg: " << std::fixed << std::setprecision(2) << avg << endl; 
                            db.Insert("hourly_stats", ts, avg);
                            db.CleanupOldData("hourly_stats", ts - RETENTION_HOUR);
                        }
                        last_hour_calc = now;
                    }

                    // проверка на дневной интервал и сохранение daily_stats
                    auto diff_d = duration_cast<seconds>(now - last_day_calc).count();
                    if (diff_d >= SIM_DAY) {
                        std::lock_guard<std::mutex> lock(db_mutex);
                        double avg = db.GetAverage("hourly_stats", ts - SIM_DAY);
                        if (avg > 0.001) {
                            cout << "[DAILY] Avg: " << std::fixed << std::setprecision(2) << avg << endl;
                            db.Insert("daily_stats", ts, avg);
                            db.CleanupOldData("daily_stats", ts - RETENTION_DAY);
                        }
                        last_day_calc = now;
                    }

                } catch (const std::exception& e) { 
                    cerr << "Parse error: " << e.what() << " on raw: " << raw << endl;
                } catch (...) {
                    cerr << "Unknown error parsing: " << raw << endl;
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: LogTool <PORT>" << endl;
        cout << "Example: LogTool COM (Windows) or LogTool /dev/ttyUSB0 (Linux)" << endl;
        return 1;
    }

    if (!db.Open("sensor_log.db")) {
        cerr << "Error opening db" << endl;
        return 1;
    }
    cout << "DB opened" << endl;

    cplib::SerialPort port;
    if (port.Open(argv[1], cplib::SerialPort::BAUDRATE_9600) != cplib::SerialPort::RE_OK) {
        cerr << "Port error: " << argv[1] << endl;
        return 1;
    }
    port.Flush();
    port.SetTimeout(0.1);
    cout << "Listening on port " << argv[1] << endl;

    HttpServer server(8080);

    server.SetHandlers(
        []() { // /api/current
            std::lock_guard<std::mutex> lock(db_mutex);
            return records_to_json(db.GetAll("raw_data", 50));
        },
        []() { // /api/hourly
            std::lock_guard<std::mutex> lock(db_mutex);
            return records_to_json(db.GetAll("hourly_stats", 24));
        },
        []() { // /api/daily
            std::lock_guard<std::mutex> lock(db_mutex);
            return records_to_json(db.GetAll("daily_stats", 30));
        }
    );

    std::thread collector(data_collection_loop, std::ref(port));
    
    server.Start();

    if (collector.joinable()) collector.join();
    
    return 0;
}