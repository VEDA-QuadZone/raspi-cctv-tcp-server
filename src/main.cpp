#include "../../include/server/TcpServer.hpp"

int main() {
    TcpServer server;
    server.setupSocket(8080);
    server.start();
    return 0;
}
