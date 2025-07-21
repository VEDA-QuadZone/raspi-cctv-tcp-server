#include "../../include/server/TcpServer.hpp"
#include "../../include/server/CommandHandler.hpp"
#include "../../include/server/ImageHandler.hpp"

#include <pthread.h>
#include <unistd.h>      // close()
#include <cstring>
#include <arpa/inet.h>   // htons, inet_ntoa
#include <iostream>
#include <sstream>
#include <filesystem>
#include <map>

// ThreadArg 구조체: 스레드에 서버 인스턴스와 클라이언트 FD 전달
struct ThreadArg {
    TcpServer* server;
    int        client_fd;
};

TcpServer::TcpServer() = default;

TcpServer::~TcpServer() {
    if (server_fd != -1) close(server_fd);
}

void TcpServer::setImageHandler(ImageHandler* handler) {
    this->imageHandler = handler;
}

void TcpServer::setCommandHandler(CommandHandler* handler) {
    this->commandHandler = handler;
}

void TcpServer::setupSocket(int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    std::cout << "[TcpServer] Listening on port " << port << std::endl;
}

void TcpServer::start() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "[TcpServer] New client fd=" << client_fd << std::endl;

        // 스레드용 인수 준비
        ThreadArg* arg = new ThreadArg{ this, client_fd };
        pthread_t tid;
        pthread_create(&tid, nullptr, &TcpServer::clientThread, arg);
        pthread_detach(tid);
    }
}

// static
void* TcpServer::clientThread(void* a) {
    auto* arg = static_cast<ThreadArg*>(a);
    TcpServer* server = arg->server;
    int fd = arg->client_fd;
    delete arg;

    server->handleClient(fd);
    close(fd);
    std::cout << "[TcpServer] Client thread for fd=" << fd << " exiting\n";
    return nullptr;
}

void TcpServer::handleClient(int client_fd) {
    std::string recvBuf;
    UploadSession upload;
    bool uploading = false;

    char buffer[4096];
    while (true) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n <= 0) break;  // disconnect or error

        // --- 파일 업로드 중 ---
        if (uploading) {
            size_t want = upload.filesize - upload.received;
            size_t write_now = std::min((size_t)n, want);
            upload.ofs.write(buffer, write_now);
            upload.received += write_now;

            if (upload.received == upload.filesize) {
                upload.ofs.close();
                std::string ok = R"({"status":"success","message":"Upload complete","filename":")"
                                 + upload.filename + R"("})";
                send(client_fd, ok.c_str(), ok.size(), 0);
                uploading = false;

                // 남은 바이트가 있으면 명령 파싱
                size_t left = n - write_now;
                if (left) recvBuf.append(buffer + write_now, left);
            }
            continue;
        }

        // --- 명령 모드: 텍스트 수신 ---
        recvBuf.append(buffer, n);

        size_t pos;
        while ((pos = recvBuf.find('\n')) != std::string::npos) {
            std::string line = recvBuf.substr(0, pos);
            recvBuf.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            std::cout << "[TcpServer] Cmd fd=" << client_fd << ": " << line << std::endl;

            // UPLOAD 헤더
            if (line.rfind("UPLOAD ", 0) == 0 && imageHandler) {
                std::istringstream iss(line);
                std::string cmd, filename;
                size_t filesize;
                iss >> cmd >> filename >> filesize;

                if (filename.empty() || filesize == 0) {
                    const char* err = R"({"status":"error","code":400,"message":"Invalid upload"})";
                    send(client_fd, err, strlen(err), 0);
                } else {
                    if (!std::filesystem::exists("images"))
                        std::filesystem::create_directory("images");

                    std::ofstream ofs("images/" + filename,
                                      std::ios::binary | std::ios::trunc);
                    if (!ofs) {
                        const char* err = R"({"status":"error","code":500,"message":"File open failed"})";
                        send(client_fd, err, strlen(err), 0);
                    } else {
                        uploading = true;
                        upload = UploadSession{filename, filesize, 0, std::move(ofs)};
                    }
                }
                // 곧바로 파일 모드로 전환
                break;
            }
            // 그 외 명령
            else if (commandHandler) {
                std::string resp = commandHandler->handle(line);
                send(client_fd, resp.c_str(), resp.size(), 0);
            }
        }
    }
}
