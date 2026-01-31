#ifndef BACKGROUND_RUN_HPP
#define BACKGROUND_RUN_HPP

#include <string>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    using process_handle_t = HANDLE;
#else
    #include <sys/types.h>
    #include <unistd.h>
    #include <sys/wait.h>
    using process_handle_t = pid_t;
#endif

using namespace std;

process_handle_t process_start(const string& path, const vector<string>& args = {});

bool process_wait(process_handle_t handle, int* exit_code);

#endif