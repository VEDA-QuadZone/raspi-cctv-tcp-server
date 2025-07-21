#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <netinet/in.h>
#include <vector>
#include <set>
#include <string>
#include <map>
#include <fstream>
#include <sys/select.h>
#include <unordered_map>

class ImageHandler; // 선언만!

struct UploadSession {
    std::string filename;
    size_t filesize;
    size_t received;
    std::ofstream ofs;
};

// 전역 변수를 꼭 전역으로 둘 필요는 없지만, 필요하다면 extern 선언!
// extern std::unordered_map<int, UploadSession> uploadSessions;

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    void setupSocket(int port);
    void start();
    void setImageHandler(ImageHandler* handler);

private:
    int server_fd;
    int fd_max;
    fd_set master_fds;

    std::set<int> client_fds;

    void acceptClient();
    bool handleClient(int client_fd); // 파일 업로드 등등 확장 가능

    void removeClient(int client_fd);

    // 멤버 변수: ImageHandler* (의존성 주입)
    ImageHandler* imageHandler = nullptr;

    // 업로드 세션 등 필요하면 멤버로 둘 수도 있음 (or 전역)
    // std::unordered_map<int, UploadSession> uploadSessions;
};

#endif // TCPSERVER_HPP
