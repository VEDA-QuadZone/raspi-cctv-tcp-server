#include "../../include/server/ImageHandler.hpp"
#include <filesystem>
#include <fstream>
#include <unistd.h> 
#include <vector>
#include <iostream>
#include <chrono>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> // read, write

ImageHandler::ImageHandler(sqlite3* db) : db(db) {}

std::string ImageHandler::handleImageUpload(int client_fd, const std::string& filename, size_t filesize) {
    // 1. 디렉토리 생성
    if (!std::filesystem::exists("images")) {
        std::filesystem::create_directory("images");
    }

    // 2. 파일명 유효성 검사 (경로 공격 방지)
    if (filename.empty() || filesize == 0 ||
        filename.find("..") != std::string::npos ||
        filename.find('/') != std::string::npos ||
        filename.find('\\') != std::string::npos) {
        return R"({"status": "error", "code": 400, "message": "Invalid filename or filesize"})";
    }

    // 3. 고유한 파일명 생성 (타임스탬프 + 원래 이름)
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::string safeFilename = std::to_string(timestamp) + "_" + filename;
    std::string fullPath = "images/" + safeFilename;

    // 4. 파일 이미 존재하는 경우 방지 (이론상 불가능하지만 안전망)
    if (std::filesystem::exists(fullPath)) {
        return R"({"status": "error", "code": 409, "message": "File already exists"})";
    }

    std::ofstream ofs(fullPath, std::ios::binary);
    if (!ofs.is_open()) {
        return R"({"status": "error", "code": 500, "message": "Failed to open file for writing"})";
    }

    // 5. 파일 수신
    size_t totalBytesRead = 0;
    std::vector<char> buffer(4096);

    while (totalBytesRead < filesize) {
        ssize_t bytesRead = read(client_fd, buffer.data(), std::min(buffer.size(), filesize - totalBytesRead));
        if (bytesRead == 0) {
            ofs.close();
            std::filesystem::remove(fullPath);
            return R"({"status": "error", "code": 499, "message": "Client closed connection"})";
        }
        if (bytesRead < 0) {
            perror("read");
            ofs.close();
            std::filesystem::remove(fullPath);
            return R"({"status": "error", "code": 500, "message": "Read error while receiving file"})";
        }
        ofs.write(buffer.data(), bytesRead);
        totalBytesRead += bytesRead;
    }

    ofs.close();
    if (!ofs) {
        std::filesystem::remove(fullPath);
        return R"({"status": "error", "code": 500, "message": "Failed to write file completely"})";
    }

    // 6. 성공 응답 (업로드된 파일 경로를 같이 전달)
    std::string successJson = R"({"status": "success", "code": 200, "message": "Image uploaded successfully", "path": ")" + fullPath + R"("})";
    return successJson;
}

void ImageHandler::handleGetImage(int client_fd, const std::string& imagePath) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file.is_open()) {
        std::string errorMsg = R"({"status": "error", "code": 404, "message": "Image not found"})";
        send(client_fd, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    char buffer[4096];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            send(client_fd, buffer, bytesRead, 0);
        }
    }

    file.close();
}
