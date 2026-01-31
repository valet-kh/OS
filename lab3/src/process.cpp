#include "process.hpp"
#include <thread>
#include <chrono>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <sys/types.h>
#endif

using std::string;

static std::atomic<bool> copy1_running{false};
static std::atomic<bool> copy2_running{false};

int Process::get_pid() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

bool Process::launch_copy(const string& copy_type) {
    if (copy_type == "copy1" && copy1_running) return false;
    if (copy_type == "copy2" && copy2_running) return false;
    
#ifdef _WIN32
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    
    string cmd_line = string("\"") + exe_path + "\" --copy " + copy_type;
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    if (CreateProcess(
        nullptr,                    // No module name
        cmd_line.data(),            // Command line
        nullptr,                    // Process handle not inheritable
        nullptr,                    // Thread handle not inheritable
        FALSE,                      // Set handle inheritance to FALSE
        0,                          // No creation flags
        nullptr,                    // Use parent's environment block
        nullptr,                    // Use parent's starting directory
        &si,                        // Pointer to STARTUPINFO
        &pi                         // Pointer to PROCESS_INFORMATION
    )) {
        CloseHandle(pi.hThread);
        
        if (copy_type == "copy1") copy1_running = true;
        else copy2_running = true;
        
        std::thread([pi, copy_type]() {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            if (copy_type == "copy1") copy1_running = false;
            else copy2_running = false;
        }).detach();
        
        return true;
    }
    return false;
    
#else

    pid_t pid = fork();
    if (pid == 0) {
        if (copy_type == "copy1") {
            execlp("./lab3", "lab3", "--copy", "copy1", NULL);
        } else {
            execlp("./lab3", "lab3", "--copy", "copy2", NULL);
        }
        exit(1);
    } else if (pid > 0) {
        if (copy_type == "copy1") copy1_running = true;
        else copy2_running = true;
        
        std::thread([pid, copy_type]() {
            int status;
            waitpid(pid, &status, 0);
            if (copy_type == "copy1") copy1_running = false;
            else copy2_running = false;
        }).detach();
        
        return true;
    }
    return false;
#endif
}

bool Process::is_copy1_running() {
    return copy1_running;
}

bool Process::is_copy2_running() {
    return copy2_running;
}
