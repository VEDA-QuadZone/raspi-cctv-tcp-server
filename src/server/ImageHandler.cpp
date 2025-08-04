// =====================
// src/server/ImageHandler.cpp
// =====================
#include "../../include/server/ImageHandler.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include <openssl/err.h>
#include "../../include/util/EndianUtils.hpp"

ImageHandler::ImageHandler(sqlite3* db) : db(db) {}

std::string ImageHandler::handleImageUpload(SSL* ssl, const std::string& filename, size_t filesize) {
    if (!std::filesystem::exists("images")) {
        std::filesystem::create_directory("images");
    }

    if (filename.empty() || filesize == 0 ||
        filename.find("..") != std::string::npos ||
        filename.find('/') != std::string::npos ||
        filename.find('\\') != std::string::npos) {
        return R"({"status":"error","code":400,"message":"Invalid filename or filesize"})";
    }

    std::string fullPath = "images/" + filename;
    if (std::filesystem::exists(fullPath)) {
        return R"({"status":"error","code":409,"message":"File already exists"})";
    }

    std::ofstream ofs(fullPath, std::ios::binary);
    if (!ofs) {
        return R"({"status":"error","code":500,"message":"Failed to open file for writing"})";
    }

    size_t totalBytesRead = 0;
    std::vector<char> buffer(4096);

    while (totalBytesRead < filesize) {
        int bytesRead = SSL_read(ssl, buffer.data(), std::min(buffer.size(), filesize - totalBytesRead));
        if (bytesRead <= 0) {
            ERR_print_errors_fp(stderr);
            ofs.close();
            std::filesystem::remove(fullPath);
            return R"({"status":"error","code":500,"message":"Read error while receiving file"})";
        }
        ofs.write(buffer.data(), bytesRead);
        totalBytesRead += bytesRead;
    }

    ofs.close();
    if (!ofs) {
        std::filesystem::remove(fullPath);
        return R"({"status":"error","code":500,"message":"Failed to write file completely"})";
    }

   return std::string("{\"status\":\"success\",\"code\":200,\"message\":\"Image uploaded successfully\",\"path\":\"")
       + fullPath + "\"}";
}

void ImageHandler::handleGetImage(SSL* ssl, const std::string& imagePath) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file.is_open()) {
        std::string errorMsg = R"({"status": "error", "code": 404, "message": "Image not found"})";
        SSL_write(ssl, errorMsg.c_str(), errorMsg.size());
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    uint64_t netFileSize = htonll(fileSize);
    if (SSL_write(ssl, &netFileSize, sizeof(netFileSize)) <= 0) {
        ERR_print_errors_fp(stderr);
        file.close();
        return;
    }

    char buffer[4096];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (SSL_write(ssl, buffer, bytesRead) <= 0) {
                ERR_print_errors_fp(stderr);
                break;
            }
        }
    }

    file.close();
}