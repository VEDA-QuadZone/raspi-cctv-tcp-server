#include "../../include/server/CommandHandler.hpp"
#include <json.hpp> // nlohmann::json 사용을 위해
#include <openssl/sha.h> // SHA-256 해시를 위해
#include <sstream>
#include <iomanip>
#include <optional>

CommandHandler::CommandHandler(sqlite3* db)
    : userRepo(db), historyRepo(db) {}

std::string CommandHandler::handle(const std::string& commandStr) {
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
        return handleGetImage(payload);
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

std::string CommandHandler::handleGetHistory(const std::string&) {
    return R"({"status": "error", "code": 501, "message": "Not Implemented"})";
}

std::string CommandHandler::handleGetHistoryByEventType(const std::string&) {
    return R"({"status": "error", "code": 501, "message": "Not Implemented"})";
}

std::string CommandHandler::handleGetHistoryByDateRange(const std::string&) {
    return R"({"status": "error", "code": 501, "message": "Not Implemented"})";
}

std::string CommandHandler::handleGetHistoryByEventTypeAndDateRange(const std::string&) {
    return R"({"status": "error", "code": 501, "message": "Not Implemented"})";
}

std::string CommandHandler::handleGetImage(const std::string&) {
    return R"({"status": "error", "code": 501, "message": "Not Implemented"})";
}
