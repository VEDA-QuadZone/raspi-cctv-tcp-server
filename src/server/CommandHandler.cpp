#include "../../include/server/CommandHandler.hpp"
#include <json.hpp> // nlohmann::json 사용을 위해
#include <openssl/sha.h> // SHA-256 해시를 위해
#include <sstream>
#include <iomanip>
#include <optional>
#include <filesystem> // 파일 경로 처리를 위해
#include <sys/socket.h> // send() 함수 사용을 위해
#include <fstream> // 파일 읽기를 위해
#include <iostream>
#include <arpa/inet.h>

CommandHandler::CommandHandler(sqlite3* db)
    : userRepo(db), historyRepo(db) {}

std::string CommandHandler::handle(const std::string& commandStr, int client_fd) {
    std::istringstream iss(commandStr);
    std::string command;
    std::getline(iss, command, ' ');
    std::string payload;
    std::getline(iss, payload);

    if (command == "REGISTER") {
        return handleRegister(payload);
    } else if (command == "LOGIN") {
        return handleLogin(payload);
    } else if (command == "RESET_PASSWORD") {
        return handleResetPassword(payload);
    } else if (command == "GET_HISTORY") {
        return handleGetHistory(payload);
    } else if (command == "GET_HISTORY_BY_EVENT_TYPE") {
        return handleGetHistoryByEventType(payload);
    } else if (command == "GET_HISTORY_BY_DATE_RANGE") {
        return handleGetHistoryByDateRange(payload);
    } else if (command == "GET_HISTORY_BY_EVENT_TYPE_AND_DATE_RANGE") {
        return handleGetHistoryByEventTypeAndDateRange(payload);
    } else if (command == "GET_IMAGE") {
        handleGetImage(client_fd, payload); // client_fd을 전달
        return ""; // 이미지 전송 후 별도의 응답 문자열 반환 없음
    } else {
        return R"({"status": "error", "code": 400, "message": "Unknown command"})";
    }
}

std::string CommandHandler::handleRegister(const std::string& payload) {
    std::istringstream iss(payload);
    std::string email, password;
    iss >> email >> password;

    if (email.empty() || password.empty()) { // 이메일 또는 비밀번호가 비어있는 경우
        return R"({"status": "error", "code": 400, "message": "Email or password is missing"})";
    }

    if (userRepo.getUserByEmail(email).has_value()) { // 이미 존재하는 이메일인지 확인
        return R"({"status": "error", "code": 409, "message": "Email already exists"})";
    }

    // 비밀번호 해싱 (SHA-256)
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::string passwordHash = ss.str();

    // 사용자 생성
    User newUser;
    newUser.email = email;
    newUser.passwordHash = passwordHash;

    // 현재 시간(ISO 8601 포맷)을 createdAt에 저장
    std::time_t t = std::time(nullptr);
    std::stringstream dateStream;
    dateStream << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    newUser.createdAt = dateStream.str();

    if (!userRepo.createUser(newUser)) { // 사용자 생성 실패
        return R"({"status": "error", "code": 500, "message": "Failed to create user"})";
    }   

    // 사용자 생성 성공
    return R"({"status": "success", "code": 200, "message": "User registered successfully"})";
}

std::string CommandHandler::handleLogin(const std::string& payload) {
    std::istringstream iss(payload);
    std::string email, password;
    iss >> email >> password;

    if (email.empty() || password.empty()) { // 이메일 또는 비밀번호가 비어있는 경우
        return R"({"status": "error", "code": 400, "message": "Email or password is missing"})";
    }

    auto userOpt = userRepo.getUserByEmail(email);

    if (!userOpt.has_value()) { // 사용자가 존재하지 않는 경우
        return R"({"status": "error", "code": 404, "message": "User not found"})";
    }

    // 사용자가 입력한 비밀번호를 SHA-256으로 해싱
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::string passwordHash = ss.str();

    // 비밀버호 비교 결과 처리
    User user = userOpt.value();
    if (user.passwordHash != passwordHash) {
        return R"({"status": "error", "code": 401, "message": "Invalid email or password"})";
    }
    
    // 로그인 성공
    return R"({"status": "success", "code": 200, "message": "Login successful"})";
}

std::string CommandHandler::handleResetPassword(const std::string& payload) {
    std::istringstream iss(payload);
    std::string email, newPassword;
    iss >> email >> newPassword;

    if (email.empty() || newPassword.empty()) { // 이메일 또는 비밀번호가 비어있는 경우
        return R"({"status": "error", "code": 400, "message": "Email or password is missing"})";
    }

    auto userOpt = userRepo.getUserByEmail(email);

    if (!userOpt.has_value()) { // 사용자가 존재하지 않는 경우
        return R"({"status": "error", "code": 404, "message": "User not found"})";
    }

    // 사용자가 입력한 새비밀번호를 SHA-256으로 해싱
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(newPassword.c_str()), newPassword.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::string newPasswordHash = ss.str();

    // 비밀번호 업데이트
    User user = userOpt.value();
    if (!userRepo.updateUserPassword(user.id, newPasswordHash)) {
        return R"({"status": "error", "code": 500, "message": "Failed to reset password"})";
    }
    
    // 비밀번호 업데이트 성공
    return R"({"status": "success", "code": 200, "message": "Password reset successful"})";
}

std::string CommandHandler::handleGetHistory(const std::string& payload) {
    std::istringstream iss(payload);
    std::string email;
    int limit = 10; // 기본값
    int offset = 0; // 기본값

    iss >> email >> limit >> offset;

    if (email.empty() || limit <= 0 || offset < 0) { // 이메일이 비어있거나 limit, offset이 유효하지 않은 경우
        return R"({"status": "error", "code": 400, "message": "Invalid input format"})";
    }

    auto userOpt = userRepo.getUserByEmail(email);
    if (!userOpt.has_value()) { // 사용자가 존재하지 않는 경우
        return R"({"status": "error", "code": 404, "message": "User not found"})";
    }

    // 사용자의 히스토리 가져오기
    auto history = historyRepo.getHistories(limit, offset);

    if (history.empty()) { // 히스토리가 없는 경우
        nlohmann::json response = {
            {"status", "success"},
            {"code", 200},
            {"message", "No history found"},
            {"data", nlohmann::json::array()}
        };
        return response.dump();
    }

    nlohmann::json data = nlohmann::json::array(); // JSON 배열 생성
    for (const auto& h : history) {
        data.push_back({
            {"id", h.id},
            {"date", h.date},
            {"image_path", h.imagePath},
            {"plate_number", h.plateNumber},
            {"event_type", h.eventType}
        });
    }

    nlohmann::json response = {
        {"status", "success"},
        {"code", 200},
        {"message", "History retrieved successfully"},
        {"data", data}
    };
    return response.dump();
}

std::string CommandHandler::handleGetHistoryByEventType(const std::string& payload) {
    std::istringstream iss(payload);
    std::string email;
    int eventType = 0;
    int limit = 10; // 기본값
    int offset = 0; // 기본값

    iss >> email >> eventType >> limit >> offset;

    if (email.empty() || eventType < 0 || limit <= 0 || offset < 0) { 
        return R"({"status": "error", "code": 400, "message": "Invalid input format"})";
    }

    auto userOpt = userRepo.getUserByEmail(email);
    if (!userOpt.has_value()) {
        return R"({"status": "error", "code": 404, "message": "User not found"})";
    }

    // 이벤트 타입에 따른 히스토리 가져오기
    auto history = historyRepo.getHistoriesByEventType(eventType, limit, offset);
    if (history.empty()) { // 히스토리가 없는 경우
        nlohmann::json response = {
            {"status", "success"},
            {"code", 200},
            {"message", "No history found"},
            {"data", nlohmann::json::array()}
        };
        return response.dump();
    }

    nlohmann::json data = nlohmann::json::array(); // JSON 배열 생성
    for (const auto& h : history) {
        data.push_back({
            {"id", h.id},
            {"date", h.date},
            {"image_path", h.imagePath},
            {"plate_number", h.plateNumber},
            {"event_type", h.eventType}
        });
    }

    nlohmann::json response = {
        {"status", "success"},
        {"code", 200},
        {"message", "History retrieved successfully"},
        {"data", data}
    };
    return response.dump();
}

std::string CommandHandler::handleGetHistoryByDateRange(const std::string& payload) {
    std::istringstream iss(payload);
    std::string email, startDateRaw, endDateRaw;
    int limit = 10;
    int offset = 0;

    iss >> email >> startDateRaw >> endDateRaw >> limit >> offset;

    std::string startDate = startDateRaw + " 00:00:00";
    std::string endDate = endDateRaw + " 23:59:59";

    if (email.empty() || startDateRaw.empty() || endDateRaw.empty() || limit <= 0 || offset < 0) {
        return R"({"status": "error", "code": 400, "message": "Invalid input format"})";
    }

    auto userOpt = userRepo.getUserByEmail(email);
    if (!userOpt.has_value()) {
        return R"({"status": "error", "code": 404, "message": "User not found"})";
    }

    auto histories = historyRepo.getHistoriesByDateRange(startDate, endDate, limit, offset);

    if (histories.empty()) {
        nlohmann::json response = {
            {"status", "success"},
            {"code", 200},
            {"message", "No history found"},
            {"data", nlohmann::json::array()}
        };
        return response.dump();
    }

    nlohmann::json data = nlohmann::json::array();
    for (const auto& h : histories) {
        data.push_back({
            {"id", h.id},
            {"date", h.date},
            {"image_path", h.imagePath},
            {"plate_number", h.plateNumber},
            {"event_type", h.eventType}
        });
    }

    nlohmann::json response = {
        {"status", "success"},
        {"code", 200},
        {"message", "History retrieved successfully"},
        {"data", data}
    };

    return response.dump();
}


std::string CommandHandler::handleGetHistoryByEventTypeAndDateRange(const std::string& payload) {
    std::istringstream iss(payload);
    std::string email, startDateRaw, endDateRaw;
    int eventType = 0;
    int limit = 10;
    int offset = 0;

    iss >> email >> eventType >> startDateRaw >> endDateRaw >> limit >> offset;

    std::string startDate = startDateRaw + " 00:00:00";
    std::string endDate = endDateRaw + " 23:59:59";

    if (email.empty() || startDateRaw.empty() || endDateRaw.empty() || eventType < 0 || limit <= 0 || offset < 0) {
        return R"({"status": "error", "code": 400, "message": "Invalid input format"})";
    }

    auto userOpt = userRepo.getUserByEmail(email);
    if (!userOpt.has_value()) {
        return R"({"status": "error", "code": 404, "message": "User not found"})";
    }

    auto histories = historyRepo.getHistoriesByEventTypeAndDateRange(eventType, startDate, endDate, limit, offset);

    if (histories.empty()) {
        nlohmann::json response = {
            {"status", "success"},
            {"code", 200},
            {"message", "No history found"},
            {"data", nlohmann::json::array()}
        };
        return response.dump();
    }

    nlohmann::json data = nlohmann::json::array();
    for (const auto& h : histories) {
        data.push_back({
            {"id", h.id},
            {"date", h.date},
            {"image_path", h.imagePath},
            {"plate_number", h.plateNumber},
            {"event_type", h.eventType}
        });
    }

    nlohmann::json response = {
        {"status", "success"},
        {"code", 200},
        {"message", "History retrieved successfully"},
        {"data", data}
    };

    return response.dump();
}

bool CommandHandler::handleGetImage(int client_fd, const std::string& payload) {
    std::string baseDir = "./images/";
    std::string fullPath = baseDir + payload;

    if (payload.empty() || !std::filesystem::exists(fullPath)) {
        std::string errorMsg = R"({"status": "error", "code": 404, "message": "Image not found"})";
        send(client_fd, errorMsg.c_str(), errorMsg.size(), 0);
        return false;
    }

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);  // ✅ 경로 수정
    if (!file.is_open()) {
        std::string errorMsg = R"({"status": "error", "code": 500, "message": "Failed to open image file"})";
        send(client_fd, errorMsg.c_str(), errorMsg.size(), 0);
        return false;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0 || fileSize > UINT32_MAX) {
        std::cerr << "[Server] Invalid file size.\n";
        return false;
    }

    file.seekg(0, std::ios::beg);

    uint32_t imageSize = static_cast<uint32_t>(fileSize);
    uint32_t imageSizeNet = htonl(imageSize);
    if (send(client_fd, &imageSizeNet, sizeof(imageSizeNet), 0) == -1) {
        std::cerr << "[Server] Failed to send image size\n";
        return false;
    }

    char buffer[4096];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (send(client_fd, buffer, bytesRead, 0) == -1) {
                std::cerr << "[Server] Failed to send image data\n";
                return false;
            }
        }
    }

    file.close();
    std::cout << "[Server] Image sent successfully.\n";
    return true;
}
