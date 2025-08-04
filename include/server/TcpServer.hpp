#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <netinet/in.h>
#include <string>
#include <map>
#include <unordered_map>
#include <fstream>
#include <openssl/ssl.h>
#include <openssl/err.h>

class ImageHandler;   // forward declaration

struct UploadSession {
    std::string filename;
    size_t      filesize;
    size_t      received;
    std::ofstream ofs;
};

class TcpServer {
public:
    // SSL_CTX* 를 받아서 보관
    explicit TcpServer(SSL_CTX* ctx);
    ~TcpServer();

    // 소켓 생성/바인드/리스닝
    void setupSocket(int port);

    // 무한 루프에서 accept() → 쓰레드 생성
    void start();

    // ImageHandler 연결
    void setImageHandler(ImageHandler* handler);
    bool handleClientSSL(int client_fd, SSL* ssl);
private:
    // SSL 연결로 들어온 클라이언트 처리
    

    int       server_fd;
    SSL_CTX*  sslCtx;             // TLS 설정 컨텍스트
    ImageHandler* imageHandler;   // 업로드 처리기
};

#endif // TCPSERVER_HPP
