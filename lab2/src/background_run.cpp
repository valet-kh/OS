#include "background_run.hpp"
#include <cstring>
#include <iostream>

#ifdef _WIN32

process_handle_t process_start(const string& path, const vector<string>& args) {

    string cmd_line = path; 

    for (const auto& arg : args) {
        cmd_line += " " + arg;  
    }
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    
    // Start the child process
    if (!CreateProcess(
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
        cout << "CreateProcess failed (" << GetLastError() << ")\n";
        return nullptr;
    }

    CloseHandle(pi.hThread);
    return pi.hProcess;
}

bool process_wait(process_handle_t handle, int* exit_code) {
	
    if (!handle) {
        return false; 
    }
    
    DWORD wait_result = WaitForSingleObject(handle, INFINITE);
    
    if (wait_result != WAIT_OBJECT_0) {
        CloseHandle(handle);
        return false;  
    }
    
    if (exit_code) {
        DWORD code;
        if (GetExitCodeProcess(handle, &code)) {
            *exit_code = code;
        } else {
            CloseHandle(handle);
            return false;
        }
    }
    
    CloseHandle(handle);
    return true;
}

#else 

process_handle_t process_start(const string& path, const vector<string>& args) {

	 // Start the child process
    pid_t pid = fork();
    
    if (pid == -1) {
        return -1; 
    }
    
    if (pid == 0) {  
        vector<char*> argv;
        
        argv.push_back(const_cast<char*>(path.c_str()));
        
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        
        // execvp требует массив char*, оканчивающийся nullptr
        argv.push_back(nullptr);
        
        execvp(path.c_str(), argv.data());
        
        exit(EXIT_FAILURE);
    }
    

    return pid;
}

bool process_wait(process_handle_t handle, int* exit_code) {
    
    if (handle <= 0) {
        return false;
    }
    
    int status;
    pid_t result = waitpid(handle, &status, 0);
    
    if (result == -1) {
        return false;
    }
    
    if (exit_code) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else {
            return false;  
        }
    }
    
    return true;
}

#endif
