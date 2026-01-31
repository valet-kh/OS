#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib> 
#include <ctime>   

#include "my_serial.hpp"

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(x) Sleep(x)
#else
    #include <unistd.h>
    #define SLEEP_MS(x) usleep((x)*1000)
#endif

void run_simulation(const char* port_name) {
    cplib::SerialPort serial;
    if (serial.Open(port_name, cplib::SerialPort::BAUDRATE_9600) != cplib::SerialPort::RE_OK) {
        std::cerr << "Cannot open port: " << port_name << std::endl;
        return;
    }

    // Инициализируем генератор случайных чисел от текущего времени
    std::srand(std::time(nullptr));

    while (true) {

        double t = 20.0 + (std::rand() % 51) / 10.0;
        
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.2f\n", t);
        
        std::string payload = buffer;
        
        if (serial.Write(payload) == cplib::SerialPort::RE_OK) {
            std::cout << "Sent: " << payload;
        }

        SLEEP_MS(1000);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    run_simulation(argv[1]);
    return 0;
}