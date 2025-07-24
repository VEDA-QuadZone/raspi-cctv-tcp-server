#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <netinet/in.h>
#include <string>
#include <map>
#include <unordered_map>
#include <fstream>

class ImageHandler; // 선언만!

struct UploadSession {
    std::string filename;
    size_t filesize;
    size_t received;
    std::ofstream ofs;
};

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    void setupSocket(int port);
    void start();                      // 클라이언트 접속 루프 (각 접속마다 pthread 생성)
    void setImageHandler(ImageHandler*);
    bool handleClient(int client_fd);  // pthread에서 실행될 클라이언트 처리 함수

private:
    int server_fd;
    ImageHandler* imageHandler = nullptr;
};

#endif // TCPSERVER_HPP
