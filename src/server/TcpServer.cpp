// src/server/TcpServer.cpp

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <iostream>
#include <cstring>

#include "server/TcpServer.hpp"
#include "server/CommandHandler.hpp"
#include "server/ImageHandler.hpp"

// main.cpp 에서 초기화된 commandHandler
extern CommandHandler* commandHandler;

// pthread 로 넘길 인자 구조체
struct ClientHandlerArgs {
    int        client_fd;
    SSL*       ssl;
    TcpServer* server;
};

// 클라이언트별 스레드 함수
static void* client_thread_func(void* arg) {
    auto* args = static_cast<ClientHandlerArgs*>(arg);
    int client_fd    = args->client_fd;
    SSL* ssl         = args->ssl;
    TcpServer* serv  = args->server;
    delete args;

    serv->handleClientSSL(client_fd, ssl);

    // 연결 종료
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client_fd);
    return nullptr;
}

TcpServer::TcpServer(SSL_CTX* ctx)
  : server_fd(-1), sslCtx(ctx), imageHandler(nullptr) {
    // SIGPIPE 방지
    signal(SIGPIPE, SIG_IGN);
}

TcpServer::~TcpServer() {
    if (server_fd != -1) close(server_fd);
}

void TcpServer::setImageHandler(ImageHandler* handler) {
    imageHandler = handler;
}

void TcpServer::setupSocket(int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "[TcpServer] Listening on port " << port << "\n";
}

void TcpServer::start() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "[TcpServer] TCP connection fd=" << client_fd << "\n";

        // SSL 핸드쉐이크
        SSL* ssl = SSL_new(sslCtx);
        SSL_set_fd(ssl, client_fd);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }
        std::cout << "[TcpServer] SSL handshake OK (fd=" << client_fd << ")\n";

        // 스레드로 분기
        auto* args = new ClientHandlerArgs{client_fd, ssl, this};
        pthread_t tid;
        if (pthread_create(&tid, nullptr, client_thread_func, args) != 0) {
            perror("pthread_create");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_fd);
            delete args;
            continue;
        }
        pthread_detach(tid);
    }
}


bool TcpServer::handleClientSSL(int client_fd, SSL* ssl) {
    while (true) {
        std::string cmd;
        char ch;
        int n;
        while (true) {
            n = SSL_read(ssl, &ch, 1);
            if (n <= 0) {
                if (n < 0) ERR_print_errors_fp(stderr);
                return false;
            }
            cmd.push_back(ch);
            if (ch == '\n') break;
        }

        if (cmd == "\n") continue;
        std::cout << "[TcpServer] Received: " << cmd;

        // UPLOAD 처리
        if (cmd.rfind("UPLOAD", 0) == 0 && imageHandler) {
            std::istringstream iss(cmd);
            std::string tag, filename;
            size_t filesize;
            iss >> tag >> filename >> filesize;

            if (filename.empty() || filesize == 0) {
                const char* err = R"({"status":"error","code":400,"message":"Invalid filename or filesize"}\n)";
                SSL_write(ssl, err, strlen(err));
            } else {
                std::string result = imageHandler->handleImageUpload(ssl, filename, filesize);
                if (result.back() != '\n') result.push_back('\n');
                SSL_write(ssl, result.c_str(), result.size());
            }
        } else if (cmd.rfind("GET_IMAGE", 0) == 0 && imageHandler != nullptr) {
    std::istringstream iss(cmd);
    std::string tag, imagePath;
    iss >> tag >> imagePath;

    if (imagePath.empty()) {
        std::string error = R"({"status": "error", "code": 400, "message": "Missing image path"})";
        SSL_write(ssl, error.c_str(), error.size());  // ✅ TLS로 전송
        return false;
    }

    imageHandler->handleGetImage(ssl, imagePath);  // ✅ SSL 기반 이미지 전송
    return true;
}
 else {
            std::string resp = commandHandler->handle(cmd);
            if (resp.back() != '\n') resp.push_back('\n');
            SSL_write(ssl, resp.c_str(), resp.size());
        }
    }
    return false;
}
