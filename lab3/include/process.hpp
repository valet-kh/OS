#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <string>
#include <atomic>

class Process {
public:
    static int get_pid();
    static bool launch_copy(const std::string& copy_type);
    
    static bool is_copy1_running();
    static bool is_copy2_running();

};

#endif