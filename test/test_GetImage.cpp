#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    const char* server_ip = "192.168.0.61";  // 라즈베리파이 서버 IP
    const int server_port = 8080;
    const std::string imagePath = "images/test.jpg";  // 테스트할 이미지 경로
    const std::string savePath = "received_test.jpg"; // 저장할 로컬 파일 경로

    // 1. 소켓 생성 및 서버 연결
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "서버 연결 실패" << std::endl;
        return 1;
    }

    // 2. GET_IMAGE 명령어 전송
    std::string command = "GET_IMAGE " + imagePath + "\n";
    send(sock, command.c_str(), command.size(), 0);

    // 3. 서버 응답 수신 및 파일로 저장
    char buffer[4096];
    std::ofstream outFile(savePath, std::ios::binary);

    int bytesRead;
    while ((bytesRead = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        // 에러 응답일 경우 JSON으로 처리
        if (bytesRead < 100 && buffer[0] == '{') {
            std::cout << "서버 응답: " << std::string(buffer, bytesRead) << std::endl;
            break;
        }
        outFile.write(buffer, bytesRead);
    }

    std::cout << "이미지 수신 완료: " << savePath << std::endl;

    close(sock);
    outFile.close();
    return 0;
}
