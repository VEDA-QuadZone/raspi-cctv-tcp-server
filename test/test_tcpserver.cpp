#include "../include/server/TcpServer.hpp"

int main() {
    TcpServer server;
    server.setupSocket(9090);  // 테스트용 포트
    server.start();            // 서버 루프 시작
    return 0;
}
