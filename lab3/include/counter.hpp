#ifndef COUNTER_HPP
#define COUNTER_HPP

#include "shared_mem.hpp"

class SharedCounter {
private:
    cplib::SharedMem<int>* shmem;
    
public:
    SharedCounter(const char* name);
    ~SharedCounter();
    
    bool is_valid() const;
    
    int get();
    void set(int value);
    void increment();
    void add(int delta);
    void multiply(int factor);
    void divide(int divisor);


    bool try_become_active(int pid);
    void release_active(int pid);
    int get_active_pid();
};

#endif