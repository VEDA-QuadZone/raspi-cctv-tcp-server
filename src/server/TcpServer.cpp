#include "../../include/server/TcpServer.hpp"
#include "../../include/server/CommandHandler.hpp"

#include <iostream>
#include <unistd.h>      // close()
#include <cstring>       // memset
#include <arpa/inet.h>   // htons, inet_ntoa
#include <sstream>       // istringstream 사용을 위해

extern CommandHandler* commandHandler; // CommandHandler 인스턴스

TcpServer::TcpServer(sqlite3* db) : server_fd(-1), fd_max(0), commandHandler(db) {
    FD_ZERO(&master_fds);
}

TcpServer::~TcpServer() {
    if (server_fd != -1) {
        close(server_fd);
    }
    for (int client_fd : client_fds) {
        close(client_fd);
    }
}

void TcpServer::setupSocket(int port) {
    // 1. 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. 주소 구조체 설정
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 3. 바인딩
    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // 4. 리슨
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 5. FD_SET 등록
    FD_SET(server_fd, &master_fds);
    fd_max = server_fd;

    std::cout << "[TcpServer] Listening on port " << port << std::endl;
}

void TcpServer::start() {
    while (true) {
        fd_set read_fds = master_fds;

        int activity = select(fd_max + 1, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            acceptClient();
        }

        std::vector<int> disconnected_fds;

        for (int client_fd : client_fds) {
            if (FD_ISSET(client_fd, &read_fds)) {
                handleClient(client_fd);
            }
        }
    }
}

void TcpServer::acceptClient() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        return;
    }

    FD_SET(client_fd, &master_fds);
    if (client_fd > fd_max) {
        fd_max = client_fd;
    }
    client_fds.insert(client_fd);

    std::cout << "[TcpServer] New client connected: fd=" << client_fd << std::endl;
}

void TcpServer::handleClient(int client_fd) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        std::cout << "[TcpServer] Client disconnected: fd=" << client_fd << std::endl;
        removeClient(client_fd);
        return;
    }

    std::string commandStr(buffer);
    std::cout << "[TcpServer] Received command: " << commandStr << std::endl;

    // 명령어 처리: CommandHandler 통해 응답 생성
    std::string response = commandHandler.handle(commandStr, client_fd);

    // 응답 전송
    send(client_fd, response.c_str(), response.size(), 0);

    // 디버그 출력
    std::cout << "[TcpServer] Sent response to fd=" << client_fd << ": " << response << std::endl;
}

void TcpServer::removeClient(int client_fd) {
    close(client_fd);
    FD_CLR(client_fd, &master_fds);
    client_fds.erase(client_fd);
}
