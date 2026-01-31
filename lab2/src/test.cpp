#include "background_run.hpp"
#include <iostream>

using namespace std;


int main(int argc, char* argv[]) {

    if (argc < 2) {
        cout << "Usage: " << argv[0] << "  [cmdline]\n";
        return 1;
    }
    
    string program = argv[1];
    
    vector<string> args;
    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }
    
    cout << "Starting: " << program << "\n";
    
    process_handle_t handle = process_start(program, args);
    
    if (!handle) {
        cout << "Failed to start process\n";
        return 1;
    }
    
    cout << "Process started. Waiting...\n";

    int exit_code;
    if (process_wait(handle, &exit_code)) {
        cout << "Process finished. Exit code: " << exit_code << "\n";
    } else {
        cout << "Error waiting for process\n";
    }
    
    return 0;
}
