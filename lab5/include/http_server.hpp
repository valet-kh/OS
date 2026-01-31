#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread> 

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <cstring>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

using HandlerFunc = std::function<std::string()>;

class HttpServer {
    socket_t _server_fd;
    bool _running;
    int _port;
    HandlerFunc _on_current_data;
    HandlerFunc _on_hourly_data;
    HandlerFunc _on_daily_data;

public:
    HttpServer(int port) : _port(port), _server_fd(INVALID_SOCKET), _running(false) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~HttpServer() {
        Stop();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void SetHandlers(HandlerFunc current, HandlerFunc hourly, HandlerFunc daily) {
        _on_current_data = current;
        _on_hourly_data = hourly;
        _on_daily_data = daily;
    }

    void Start() {
        _server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_server_fd == INVALID_SOCKET) return;

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(_port);

#ifndef _WIN32
        int opt = 1;
        setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        if (bind(_server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
            std::cerr << "[SERVER] Bind failed on port " << _port << "\n";
            return;
        }

        if (listen(_server_fd, 5) == SOCKET_ERROR) {
            std::cerr << "[SERVER] Listen failed\n";
            return;
        }

        _running = true;
        std::cout << "[SERVER] Started on http://localhost:" << _port << std::endl;
        std::cout << "[SERVER] Open web_client.html in your browser or go to http://localhost:" << _port << "/web_client.html" << std::endl;

        while (_running) {
            sockaddr_in client_addr;
#ifdef _WIN32
            int addrlen = sizeof(client_addr);
#else
            socklen_t addrlen = sizeof(client_addr);
#endif
            socket_t new_socket = accept(_server_fd, (struct sockaddr*)&client_addr, &addrlen);
            
            if (new_socket != INVALID_SOCKET) {
                // Используем стандартный std::thread и detach, чтобы он работал в фоне
                std::thread(&HttpServer::HandleClient, this, new_socket).detach();
            }
        }
    }

    void Stop() {
        _running = false;
        if (_server_fd != INVALID_SOCKET) closesocket(_server_fd);
    }

private:
    void HandleClient(socket_t client_socket) {
        char buffer[4096] = {0};
        recv(client_socket, buffer, 4096, 0);
        
        std::string request(buffer);
        std::string response;
        std::string content_type = "text/plain";
        std::string body = "";

        // Простой роутинг
        if (request.find("GET /api/current") != std::string::npos) {
            content_type = "application/json";
            if(_on_current_data) body = _on_current_data();
            else body = "[]";
        } 
        else if (request.find("GET /api/hourly") != std::string::npos) {
            content_type = "application/json";
            if(_on_hourly_data) body = _on_hourly_data();
            else body = "[]";
        } 
        else if (request.find("GET /api/daily") != std::string::npos) {
            content_type = "application/json";
            if(_on_daily_data) body = _on_daily_data();
            else body = "[]";
        }
        else if (request.find("GET / ") != std::string::npos || request.find("GET /web_client.html") != std::string::npos) {
            content_type = "text/html";
            std::ifstream f("web_client.html"); 
            if (f.good()) {
                std::stringstream buffer;
                buffer << f.rdbuf();
                body = buffer.str();
            } else {
                body = "<html><body><h1>File web_client.html not found!</h1><p>Ensure it is in the same folder as the executable.</p></body></html>";
            }
        }
        else {
            body = "404 Not Found";
        }

        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: " + content_type + "\r\n";
        response += "Access-Control-Allow-Origin: *\r\n"; // Разрешаем CORS для локальной разработки
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response += "Connection: close\r\n\r\n";
        response += body;

        send(client_socket, response.c_str(), response.size(), 0);
        closesocket(client_socket);
    }
};