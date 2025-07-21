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
#include <chrono>
#include <filesystem>
extern CommandHandler* commandHandler; // CommandHandler 인스턴스
std::map<int, std::string> recvBuffers;
std::unordered_map<int, UploadSession> uploadSessions;
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

// TcpServer.cpp
/*
bool TcpServer::handleClient(int fd) {
    char buf[4096];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n == 0) return true;
    if (n < 0) { perror("[TcpServer] recv error"); return true; }

    // 1. 파일 업로드 중이면
    if (uploadSessions.count(fd)) {
        auto& sess = uploadSessions[fd];
        size_t need = sess.filesize - sess.received;
        size_t to_write = std::min((size_t)n, need);
        sess.ofs.write(buf, to_write);
        sess.received += to_write;

        // 다 받았으면
        if (sess.received == sess.filesize) {
            sess.ofs.close();
            std::string ok = R"({"status": "success", "message": "upload ok", "filename": ")" + sess.filename + R"("})";
            send(fd, ok.c_str(), ok.size(), 0);
            uploadSessions.erase(fd);

            // 남은 데이터(혹시 next command?) 있으면 커맨드 버퍼에 넣음
            size_t leftover = n - to_write;
            if (leftover > 0)
                recvBuffers[fd].append(buf + to_write, leftover);
        } else {
            // 아직 업로드 중: 명령 파싱 절대 X, 리턴
            return false;
        }
        // 업로드 막 끝났으면 (위에서 append한 남은 버퍼만큼)
        // 아래에서 바로 커맨드 모드 처리
    } else {
        // 명령 모드: 텍스트 누적
        recvBuffers[fd].append(buf, n);
    }

    // 2. 명령어(줄 단위) 파싱
    while (true) {
        size_t nl = recvBuffers[fd].find('\n');
        if (nl == std::string::npos) break;
        std::string line = recvBuffers[fd].substr(0, nl);
        recvBuffers[fd].erase(0, nl + 1);

        if (line.empty()) continue;
        std::cout << "[TcpServer] Received command: " << line << std::endl;

        // 업로드 명령
        if (line.rfind("UPLOAD", 0) == 0) {
            std::istringstream iss(line);
            std::string cmd, filename; size_t filesize;
            iss >> cmd >> filename >> filesize;
            if (filename.empty() || filesize == 0) {
                std::string err = R"({"status":"error","message":"bad upload cmd"})";
                send(fd, err.c_str(), err.size(), 0);
            } else {
                if (!std::filesystem::exists("images")) std::filesystem::create_directory("images");
                std::string fpath = "images/" + filename;
                std::ofstream ofs(fpath, std::ios::binary);
                if (!ofs.is_open()) {
                    std::string err = R"({"status":"error","message":"file open failed"})";
                    send(fd, err.c_str(), err.size(), 0);
                } else {
                    uploadSessions[fd] = UploadSession{filename, filesize, 0, std::move(ofs)};
                    // 업로드 세션 등록 후 break (이제부턴 다시 파일데이터 받으러 감)
                    break;
                }
            }
            // 업로드 명령 왔으면 무조건 break (이후 데이터는 명령아님)
            break;
        }

        // add_history 등 일반 명령 처리 (여기선 echo만 예시)
        std::string resp = "[ECHO] " + line + "\n";
        send(fd, resp.c_str(), resp.size(), 0);
    }
    return false;
}
*/



void TcpServer::removeClient(int client_fd) {
    close(client_fd);
    FD_CLR(client_fd, &master_fds);
    client_fds.erase(client_fd);
    recvBuffers.erase(client_fd); // 버퍼도 함께 삭제!
}
