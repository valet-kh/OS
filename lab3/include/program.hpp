#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <atomic>
#include <thread>
#include <chrono>

#include "logger.hpp"
#include "counter.hpp"

class Program {
private:
    Logger logger;
    SharedCounter counter;
    int pid;
    
    std::atomic<bool> running{true};
    std::atomic<bool> is_active{false};
    

    std::chrono::steady_clock::time_point last_counter_update;
    std::chrono::steady_clock::time_point last_log_write;
    std::chrono::steady_clock::time_point last_launch;
    
    std::thread counter_thread;
    std::thread logger_thread;
    std::thread copy_launcher_thread;
    std::thread input_thread;
    std::thread activity_thread;
    
    void counter_loop();
    void logger_loop();
    void copy_launcher_loop();
    void input_loop();
    void activity_check_loop();
    
    void launch_copies();
    
public:
    Program();
    ~Program();
    
    void run();
    void stop();
    
    static void run_copy1();
    static void run_copy2();
};

#endif