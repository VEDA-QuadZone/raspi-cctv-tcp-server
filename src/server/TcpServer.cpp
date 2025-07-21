#include "../../include/server/TcpServer.hpp"
#include "../../include/server/CommandHandler.hpp"
#include "../../include/server/ImageHandler.hpp"

#include <iostream>
#include <unistd.h>      // close()
#include <cstring>       // memset
#include <arpa/inet.h>   // htons, inet_ntoa
#include <sstream>       // istringstream 사용을 위해
#include <map>
#include <string>
extern CommandHandler* commandHandler; // CommandHandler 인스턴스
std::map<int, std::string> recvBuffers;
TcpServer::TcpServer() : server_fd(-1), fd_max(0) {
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

void TcpServer::setImageHandler(ImageHandler* handler) {
    this->imageHandler = handler;
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
/*
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
}*/
/*
void TcpServer::start() {
    while (true) {
        fd_set read_fds = master_fds;
        int activity = select(fd_max + 1, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            perror("select");
            break;
        }

        // 1) 새로운 접속
        if (FD_ISSET(server_fd, &read_fds)) {
            acceptClient();
        }

        // 2) 기존 클라이언트 I/O 체크
        std::vector<int> disconnected_fds;
        for (int client_fd : client_fds) {
            if (FD_ISSET(client_fd, &read_fds)) {
                // true 리턴 시 “끊김”이므로 나중에 제거
                bool disconnected = handleClient(client_fd);
                if (disconnected) {
                    disconnected_fds.push_back(client_fd);
                }
            }
        }

        // 3) 안전하게 FD 제거
        for (int fd : disconnected_fds) {
            removeClient(fd);
        }
    }
}*/
void TcpServer::start() {
    while (true) {
        fd_set read_fds = master_fds;
        int activity = select(fd_max + 1, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            if (errno == EINTR) continue; // 인터럽트 신호는 무시하고 루프 계속
            perror("select");
            sleep(1); // 심각한 에러라도 1초 쉬고 다시 반복
            continue;
        }

        // 1) 새로운 접속
        if (FD_ISSET(server_fd, &read_fds)) {
            acceptClient();
        }

        // 2) 기존 클라이언트 I/O 체크
        std::vector<int> disconnected_fds;
        for (int client_fd : client_fds) {
            if (FD_ISSET(client_fd, &read_fds)) {
                bool disconnected = handleClient(client_fd);
                if (disconnected) {
                    disconnected_fds.push_back(client_fd);
                }
            }
        }

        for (int fd : disconnected_fds) {
            removeClient(fd);
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
/*
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
    std::cout << "[TcpServer] Received command: " << commandStr;

    // UPLOAD 명령어 분기 처리
    if (commandStr.rfind("UPLOAD", 0) == 0 && imageHandler != nullptr) {
        std::istringstream iss(commandStr);
        std::string cmd, filename;
        size_t filesize;
        iss >> cmd >> filename >> filesize;

        if (filename.empty() || filesize == 0) {
            std::string error = R"({"status": "error", "code": 400, "message": "Invalid filename or filesize"})";
            send(client_fd, error.c_str(), error.size(), 0);
            return;
        }

        std::string result = imageHandler->handleImageUpload(client_fd, filename, filesize);
        send(client_fd, result.c_str(), result.size(), 0);
        return;
    }
    
    // 기본 명령어 처리: CommandHandler 통해 응답 생성
    std::string response = commandHandler->handle(commandStr);

    // 응답 전송
    send(client_fd, response.c_str(), response.size(), 0);
}
*/
/*
bool TcpServer::handleClient(int client_fd) {
    // 1) 헤더(명령어 한 줄)만 읽기
    std::string commandStr;
    char ch;
    while (true) {
        ssize_t n = recv(client_fd, &ch, 1, 0);
        if (n <= 0) {
            // 클라이언트가 닫았거나 에러
            std::cout << "[TcpServer] Client disconnected: fd=" << client_fd << std::endl;
            return true;
        }
        commandStr.push_back(ch);
        if (ch == '\n') break;    // 한 줄 끝
    }

    std::cout << "[TcpServer] Received command: " << commandStr;

    // 2) UPLOAD 처리
    if (commandStr.rfind("UPLOAD", 0) == 0 && imageHandler) {
        std::istringstream iss(commandStr);
        std::string cmd, filename;
        size_t filesize;
        iss >> cmd >> filename >> filesize;

        if (filename.empty() || filesize == 0) {
            const char* err = R"({"status":"error","code":400,"message":"Invalid filename or filesize"})";
            send(client_fd, err, strlen(err), 0);
        } else {
            // handleImageUpload 은 바로 client_fd 로부터 본문을 읽습니다
            std::string result = imageHandler->handleImageUpload(client_fd, filename, filesize);
            send(client_fd, result.c_str(), result.size(), 0);
        }
        return false;
    }

    // 3) 나머지 커맨드는 그대로
    std::string response = commandHandler->handle(commandStr);
    send(client_fd, response.c_str(), response.size(), 0);
    return false;
}
*/
/*
bool TcpServer::handleClient(int client_fd) {
    while (true) {
        std::string commandStr;
        char ch;

        // 한 줄(명령어) 단위로 읽기
        while (true) {
            ssize_t n = recv(client_fd, &ch, 1, 0);
            if (n == 0) {
                // 클라이언트가 정상적으로 연결 종료(EOF)
                std::cout << "[TcpServer] Client disconnected: fd=" << client_fd << std::endl;
                return true; // 이 fd는 removeClient에서 닫힘
            }
            if (n < 0) {
                // 에러
                perror("[TcpServer] recv error");
                return true;
            }
            commandStr.push_back(ch);
            if (ch == '\n') break; // 한 줄 끝
        }

        // 공백/빈 줄 무시
        if (commandStr.empty() || commandStr == "\n") continue;

        std::cout << "[TcpServer] Received command: " << commandStr;

        // 1) UPLOAD 처리
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
            continue; // 다음 명령 읽기 (연결 종료 아님)
        }

        // 2) 나머지 커맨드는 commandHandler
        std::string response = commandHandler->handle(commandStr);
        send(client_fd, response.c_str(), response.size(), 0);
        // continue로 다음 명령 계속!
    }
    // 여기는 절대 도달 안함
    // return true; // 필요시
}
*/
bool TcpServer::handleClient(int client_fd) {
    char buf[1024];
    ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
    if (n == 0) {
        std::cout << "[TcpServer] Client disconnected: fd=" << client_fd << std::endl;
        recvBuffers.erase(client_fd);
        return true;
    }
    if (n < 0) {
        perror("[TcpServer] recv error");
        recvBuffers.erase(client_fd);
        return true;
    }
    // 1. 새 데이터 버퍼에 이어붙임
    recvBuffers[client_fd].append(buf, n);

    // 2. 줄 단위 명령 처리
    size_t pos = 0;
    while (true) {
        size_t newline = recvBuffers[client_fd].find('\n', pos);
        if (newline == std::string::npos) break; // 더 이상 줄 없음

        // 한 줄 추출
        std::string commandStr = recvBuffers[client_fd].substr(pos, newline - pos);
        pos = newline + 1;

        // 공백/빈 줄 무시
        if (commandStr.empty()) continue;

        std::cout << "[TcpServer] Received command: " << commandStr << std::endl;

        // UPLOAD, 나머지 커맨드 분기 처리 (앞서 설명한 로직 그대로!)
        // ... (commandHandler, imageHandler 등) ...
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

    // 3. 남은 데이터는 버퍼 앞으로 이동(아직 \n을 못 만난 데이터)
    if (pos > 0)
        recvBuffers[client_fd].erase(0, pos);

    return false;
}

void TcpServer::removeClient(int client_fd) {
    close(client_fd);
    FD_CLR(client_fd, &master_fds);
    client_fds.erase(client_fd);
    recvBuffers.erase(client_fd); // 버퍼도 함께 삭제!
}
