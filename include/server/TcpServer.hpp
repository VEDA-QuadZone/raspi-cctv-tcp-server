#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <netinet/in.h>  // sockaddr_in
#include <vector>
#include <set>
#include <string>
#include <sys/select.h>

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    void setupSocket(int port);
    void start();

private:
    int server_fd;
    int fd_max;
    fd_set master_fds;

    std::set<int> client_fds;

    void acceptClient();
    void handleClient(int client_fd);
    void removeClient(int client_fd);
};

#endif // TCPSERVER_HPP
