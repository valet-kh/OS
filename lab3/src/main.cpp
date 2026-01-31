#include <iostream>
#include <cstring>

#include "program.hpp"
#include "process.hpp"

int main(int argc, char* argv[]) {
    
    if (argc >= 3 && strcmp(argv[1], "--copy") == 0) {
        //std::cout << "Running as copy: " << argv[2] << std::endl;
        if (strcmp(argv[2], "copy1") == 0) {
            Program::run_copy1();
        } else if (strcmp(argv[2], "copy2") == 0) {
            Program::run_copy2();
        }
        return 0;
    }

    Program program;
 
    program.run();
    
    return 0;
}