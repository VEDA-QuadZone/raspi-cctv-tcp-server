#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <netinet/in.h>
#include <string>
#include <fstream>
#include <unordered_map>

// forward declarations
class ImageHandler;
class CommandHandler;

// 업로드 세션 정보
struct UploadSession {
    std::string filename;
    size_t      filesize;
    size_t      received;
    std::ofstream ofs;
};

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    // DI: ImageHandler, CommandHandler
    void setImageHandler(ImageHandler* handler);
    void setCommandHandler(CommandHandler* handler);

    // 서버 소켓 생성 및 바인딩
    void setupSocket(int port);

    // accept 루프 → 스레드 생성
    void start();

private:
    int server_fd = -1;
    ImageHandler*   imageHandler   = nullptr;
    CommandHandler* commandHandler = nullptr;

    // 스레드 진입점
    static void* clientThread(void* arg);

    // 각 클라이언트 전용 처리 함수
    void handleClient(int client_fd);
};

#endif // TCPSERVER_HPP
