#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    const char* server_ip = "192.168.0.61";  // 또는 Raspberry Pi의 IP
    const int server_port = 8080;
    const std::string filename = "test.jpg";  // 전송할 파일
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "파일을 열 수 없습니다: " << filename << std::endl;
        return 1;
    }

    // 파일 크기 계산
    std::streamsize filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    // 서버 연결
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "서버 연결 실패" << std::endl;
        return 1;
    }

    // 명령어 전송 (형식: UPLOAD filename filesize\n)
    std::string header = "UPLOAD " + filename + " " + std::to_string(filesize) + "\n";
    send(sock, header.c_str(), header.size(), 0);

    // 이미지 데이터 전송
    char buffer[4096];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        send(sock, buffer, bytesRead, 0);
    }

    // 응답 수신
    char response[1024] = {0};
    recv(sock, response, sizeof(response), 0);
    std::cout << "서버 응답: " << response << std::endl;

    close(sock);
    return 0;
}
