#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib> 
#include <ctime>   
#include <cstdint>

#include "my_serial.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::snprintf;
using std::rand;
using std::srand;
using std::time;

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
        cerr << "Cannot open port: " << port_name << endl;
        return;
    }

    srand(time(nullptr));
    

    while (true) {
        double t = 10.0 + (rand() % 101) / 10.0;
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f\n", t);
        
        string payload = buffer;
        
        if (serial.Write(payload) == cplib::SerialPort::RE_OK) {
            cout << "Sent: " << payload;
        }

        SLEEP_MS(1000);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: sensor_emulator <PORT>" << endl;
        cout << "Example: sensor_emulator COM№ (Windows) or /dev/ttyUSB0 (Linux)" << endl;
        return 1;
    }
    run_simulation(argv[1]);
    return 0;
}