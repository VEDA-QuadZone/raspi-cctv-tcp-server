#include "../../include/server/TcpServer.hpp"
#include "../../include/server/CommandHandler.hpp"
#include "../../include/server/ImageHandler.hpp"

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sstream>
#include <map>
#include <string>
#include <chrono>
#include <filesystem>
#include <pthread.h>

extern CommandHandler* commandHandler;
std::map<int, std::string> recvBuffers;
std::unordered_map<int, UploadSession> uploadSessions;

// 클라이언트 핸들링용 구조체
struct ClientHandlerArgs {
    int client_fd;
    TcpServer* server;
};

// pthread 전용 핸들러 함수
void* client_thread_func(void* arg) {
    ClientHandlerArgs* args = static_cast<ClientHandlerArgs*>(arg);
    int client_fd = args->client_fd;
    TcpServer* server = args->server;
    delete args;

    server->handleClient(client_fd);
    close(client_fd);
    return nullptr;
}

TcpServer::TcpServer() : server_fd(-1) {}

TcpServer::~TcpServer() {
    if (server_fd != -1) {
        close(server_fd);
    }
}

void TcpServer::setImageHandler(ImageHandler* handler) {
    this->imageHandler = handler;
}

void TcpServer::setupSocket(int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "[TcpServer] Listening on port " << port << std::endl;
}

void TcpServer::start() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "[TcpServer] New client connected: fd=" << client_fd << std::endl;

        auto* args = new ClientHandlerArgs{client_fd, this};
        pthread_t tid;
        if (pthread_create(&tid, nullptr, client_thread_func, args) != 0) {
            perror("pthread_create");
            close(client_fd);
            delete args;
            continue;
        }
        pthread_detach(tid);
    }
}

bool TcpServer::handleClient(int client_fd) {
    while (true) {
        std::string commandStr;
        char ch;

        while (true) {
            ssize_t n = recv(client_fd, &ch, 1, 0);
            if (n == 0) {
                std::cout << "[TcpServer] Client disconnected: fd=" << client_fd << std::endl;
                return true;
            }
            if (n < 0) {
                perror("[TcpServer] recv error");
                return true;
            }
            commandStr.push_back(ch);
            if (ch == '\n') break;
        }

        if (commandStr.empty() || commandStr == "\n") continue;

        std::cout << "[TcpServer] Received command: " << commandStr;

        if (commandStr.rfind("UPLOAD", 0) == 0 && imageHandler) {
            std::istringstream iss(commandStr);
            std::string cmd, filename;
            size_t filesize;
            iss >> cmd >> filename >> filesize;

            if (filename.empty() || filesize == 0) {
                const char* err = R"({"status":"error","code":400,"message":"Invalid filename or filesize"})";
                send(client_fd, err, strlen(err), 0);
            } else {
                std::string result = imageHandler->handleImageUpload(client_fd, filename, filesize);
                send(client_fd, result.c_str(), result.size(), 0);
            }
            continue;
        }

        std::string response = commandHandler->handle(commandStr);
        send(client_fd, response.c_str(), response.size(), 0);
    }
    return false;
}
