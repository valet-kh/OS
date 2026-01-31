#include "program.hpp"
#include "process.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>      
#include <sys/types.h> 

using namespace std::chrono;
using std::this_thread::sleep_for;

Program::Program() 
    : logger("program.log")
    , counter("lab_counter")
    , pid(Process::get_pid())
{
    
    logger.write("Start, PID=%d", pid);
    
    if (!counter.is_valid()) {
        logger.write("ERROR: counter is not valid");
        running = false;
        return;
    }
    
    
    if (counter.try_become_active(pid)) {
        is_active = true;
        logger.write("---- Im ACTIVE prog PID=%d", pid);
    } else {
        is_active = false;
    }
    
    last_counter_update = steady_clock::now();
    last_log_write = steady_clock::now();
    last_launch = steady_clock::now();
}

Program::~Program() {
    stop();
    if (is_active) {
        counter.release_active(pid);
    }
}

void Program::stop() {
    running = false;
}

void Program::run() {

    if (!running) return;
    
    counter_thread = std::thread(&Program::counter_loop, this);
    logger_thread = std::thread(&Program::logger_loop, this);
    copy_launcher_thread = std::thread(&Program::copy_launcher_loop, this);
    input_thread = std::thread(&Program::input_loop, this);
    
    if (!is_active) {
        activity_thread = std::thread(&Program::activity_check_loop, this);
    }
    
    // ждем
    counter_thread.join();
    logger_thread.join();
    copy_launcher_thread.join();
    input_thread.join();
    
    if (activity_thread.joinable()) {
        activity_thread.join();
    }
}

void Program::counter_loop() {
    while (running) {
        auto now = steady_clock::now();
        if (now - last_counter_update >= milliseconds(300)) {
            counter.increment();
            last_counter_update = now;
        }
        sleep_for(milliseconds(10));
    }
}

void Program::logger_loop() {
    while (running) {
        if (is_active) {
            auto now = steady_clock::now();
            if (now - last_log_write >= seconds(1)) {
                logger.write("PID=%d, cnt=%d", pid, counter.get());
                last_log_write = now;
            }
        }
        sleep_for(milliseconds(100));
    }
}

void Program::copy_launcher_loop() {
    while (running) {
        if (is_active) {
            auto now = steady_clock::now();
            if (now - last_launch >= seconds(3)) {
                launch_copies();
                last_launch = now;
            }
        }
        sleep_for(milliseconds(100));
    }
}

void Program::launch_copies() {
    if (!Process::launch_copy("copy1")) {
        logger.write("Copy1 is still running, skipping start");
    }
    
    if (!Process::launch_copy("copy2")) {
        logger.write("Copy2 is still running, skipping start");
    }
}

void Program::input_loop() {
    std::string command;
    
    std::cout << "Commands: 'show' (s) - show counter, 'status' (st) - show program status, 'set <int>' - set counter, 'exit' (e) - exit" << std::endl;
    
    while (running) {
        std::cout << "> ";
        std::cin >> command;
        
        if (command == "show" || command == "s") {
            int val = counter.get();
            std::cout << "Counter: " << val << std::endl;
        }
        else if (command == "status" || command == "st") {
            std::cout << "Program status: ";
            if (is_active) {
                std::cout << "ACTIVE" << std::endl;
            } else {
                std::cout << "PASSIVE" << std::endl;
            }
        }
        else if (command == "set" || command == "m") {
            int value;
            if (std::cin >> value) {
                counter.set(value);
                logger.write("User set counter = %d", value);
                std::cout << "Counter set to: " << value << std::endl;
            } else {
                std::cout << "Error! Enter an int." << std::endl;
                std::cin.clear();
                std::cin.ignore(10000, '\n');
            }
        }
        else if (command == "exit" || command == "e" ) {
            std::cout << "Exiting program..." << std::endl;
            running = false;
            break;
        }
        else {
            std::cout << "Unknown command!" << std::endl;
            std::cout << "Available commands: show (s), status (st), set <number> (m <number>), exit (e)" << std::endl;
        }
    }
}

void Program::activity_check_loop() {
    while (running && !is_active) {
        int active_pid = counter.get_active_pid();
        
        if (active_pid == 0) {
            if (counter.try_become_active(pid)) {
                is_active = true;
                logger.write("---- Im new ACTIVE prog PID=%d", pid);
                break;
            }
        } else {
#ifdef _WIN32
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, active_pid);
            if (hProcess) {
                DWORD exitCode;
                GetExitCodeProcess(hProcess, &exitCode);
                CloseHandle(hProcess);
                if (exitCode != STILL_ACTIVE) {
                    counter.release_active(active_pid); 
                    continue;
                }
            } else {
                counter.release_active(active_pid);
                continue;
            }
#else
            if (kill(active_pid, 0) != 0) {
                counter.release_active(active_pid);
                continue;
            }
#endif
        }
        sleep_for(milliseconds(100));
    }
}

void Program::run_copy1() {
    Logger log("program.log");
    SharedCounter cnt("lab_counter");
    
    if (!cnt.is_valid()) return;
    
    int copy_pid = Process::get_pid();
    log.write("Copy1 [PID=%d] start", copy_pid);
    
    cnt.add(10);
    
    log.write("Copy1 [PID=%d] end, cnt=%d", 
              copy_pid, cnt.get());
}

void Program::run_copy2() {
    Logger log("program.log");
    SharedCounter cnt("lab_counter");
    
    if (!cnt.is_valid()) return;
    
    int copy_pid = Process::get_pid();
    log.write("Copy2 [PID=%d] start", copy_pid);
    
    cnt.multiply(2);
    
    log.write("Copy2 [PID=%d] (x2) cnt=%d", 
              copy_pid, cnt.get());
    
    sleep_for(seconds(2));
    cnt.divide(2);
    
    log.write("Copy2 [PID=%d] end, cnt=%d", 
              copy_pid, cnt.get());
}