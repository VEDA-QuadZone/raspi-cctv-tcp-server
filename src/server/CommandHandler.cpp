#include "../../include/server/CommandHandler.hpp"
#include <json.hpp> // nlohmann::json 사용을 위해
#include <openssl/sha.h> // SHA-256 해시를 위해
#include <sstream>
#include <iomanip>
#include <optional>
#include <filesystem> // 파일 경로 처리를 위해
#include <fstream>

CommandHandler::CommandHandler(sqlite3* db)
    : userRepo(db), historyRepo(db) {}

std::string CommandHandler::handle(const std::string& commandStr) {
    std::istringstream iss(commandStr);
    std::string command;
    std::getline(iss, command, ' ');

    //command.erase(std::remove_if(command.begin(), command.end(), ::isspace), command.end());
    //std::cout << "[DEBUG] command=[" << command << "]" << std::endl;
    command.erase(0, command.find_first_not_of(" \t\r\n"));
    command.erase(command.find_last_not_of(" \t\r\n") + 1);
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
    } else if (command == "ADD_HISTORY") {
        return handleAddHistory(payload);
    } else if (command == "GET_HISTORY_BY_EVENT_TYPE") {
        return handleGetHistoryByEventType(payload);
    } else if (command == "GET_HISTORY_BY_DATE_RANGE") {
        return handleGetHistoryByDateRange(payload);
    } else if (command == "GET_HISTORY_BY_EVENT_TYPE_AND_DATE_RANGE") {
        return handleGetHistoryByEventTypeAndDateRange(payload);
    } else if (command == "GET_IMAGE") {
        return handleGetImage(payload);
    }
    else if (command == "CHANGE_FRAME") {
        return handleChangeFrame(payload);
    }
    else if (command == "GET_FRAME") {
        return handleGetFrame(payload);  
    }
    else {
        return R"({"status": "error", "code": 400, "message": "Unknown commandD"})";
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

    if (email.empty() || limit <= 0 || offset < 0) {
        return R"({"status": "error", "code": 400, "message": "Invalid input format"})";
    }

    auto userOpt = userRepo.getUserByEmail(email);
    if (!userOpt.has_value()) {
        return R"({"status": "error", "code": 404, "message": "User not found"})";
    }

    auto history = historyRepo.getHistories(limit, offset);

    nlohmann::json data = nlohmann::json::array();
    for (const auto& h : history) {
        std::string start_snapshot, end_snapshot;
        nlohmann::json speed_json;

        if (h.eventType == 0) { // 불법주정차
            start_snapshot = h.startSnapshot;
            end_snapshot = h.endSnapshot;
            speed_json = nullptr;
        } else if (h.eventType == 1) { // 과속
    start_snapshot = "";
    end_snapshot = "";
    if (h.speed.has_value()) {
        float rounded = std::round(h.speed.value() * 100) / 100.0f;
        speed_json = rounded;
    } else {
        speed_json = nullptr;
    }
} else { // 어린이 감지 등 기타
            start_snapshot = "";
            end_snapshot = "";
            speed_json = nullptr;
        }

        data.push_back({
            {"id", h.id},
            {"date", h.date},
            {"image_path", h.imagePath},
            {"plate_number", h.plateNumber},
            {"event_type", h.eventType},
            {"start_snapshot", start_snapshot},
            {"end_snapshot", end_snapshot},
            {"speed", speed_json}
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


std::string CommandHandler::handleAddHistory(const std::string& payload) {
    std::istringstream iss(payload);
    std::string rawDate, imagePath, plateNumber, startSnapshot, endSnapshot;
    int eventType;
    float speed = -1;
    iss >> rawDate >> imagePath >> plateNumber >> eventType >> startSnapshot >> endSnapshot >> speed;


    if (rawDate.empty() || imagePath.empty() || plateNumber.empty() || eventType < 0 || eventType > 2) {
        return R"({"status": "error", "code": 400, "message": "Invalid input format"})";
    }

    // 날짜 형식 변경: YYYY-MM-DD_HH:MM:SS → YYYY-MM-DD HH:MM:SS
    std::replace(rawDate.begin(), rawDate.end(), '_', ' ');

    History newHistory;
    newHistory.date = rawDate;
    newHistory.imagePath = imagePath;
    newHistory.plateNumber = plateNumber;
    newHistory.eventType = eventType;
    newHistory.startSnapshot = startSnapshot;
    newHistory.endSnapshot = endSnapshot;
    if (eventType == 1 && speed >= 0) // 속도위반일 때만
        newHistory.speed = speed;
    else
        newHistory.speed = std::nullopt;

    if (!historyRepo.createHistory(newHistory)) {
        return R"({"status": "error", "code": 500, "message": "Failed to create history"})";
    }

    return R"({"status": "success", "code": 200, "message": "History created successfully"})";
}



std::string CommandHandler::handleGetHistoryByEventType(const std::string& payload) {
    std::istringstream iss(payload);
    std::string email;
    int eventType = 0;
    int limit = 10;
    int offset = 0;

    iss >> email >> eventType >> limit >> offset;

    if (email.empty() || eventType < 0 || limit <= 0 || offset < 0) {
        return R"({"status": "error", "code": 400, "message": "Invalid input format"})";
    }

    auto userOpt = userRepo.getUserByEmail(email);
    if (!userOpt.has_value()) {
        return R"({"status": "error", "code": 404, "message": "User not found"})";
    }

    auto history = historyRepo.getHistoriesByEventType(eventType, limit, offset);

    nlohmann::json data = nlohmann::json::array();
    for (const auto& h : history) {
        if (h.eventType == 0) { // 불법주정차
            data.push_back({
                {"id", h.id},
                {"date", h.date},
                {"image_path", h.imagePath},
                {"plate_number", h.plateNumber},
                {"event_type", h.eventType},
                {"start_snapshot", h.startSnapshot},
                {"end_snapshot", h.endSnapshot}
                // speed 없음
            });
        } else if (h.eventType == 1) { // 과속
            nlohmann::json obj = {
                {"id", h.id},
                {"date", h.date},
                {"image_path", h.imagePath},
                {"plate_number", h.plateNumber},
                {"event_type", h.eventType}
                // start_snapshot, end_snapshot 없음
            };
            if (h.speed.has_value()) {
                float rounded = std::round(h.speed.value() * 100) / 100.0f;
                obj["speed"] = rounded;
            }
            data.push_back(obj);
        } else if (h.eventType == 2) { // 어린이 감지
            data.push_back({
                {"id", h.id},
                {"date", h.date},
                {"image_path", h.imagePath},
                {"event_type", h.eventType}
                // speed, start_snapshot, end_snapshot, plate_number 없음
            });
        }
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

    nlohmann::json data = nlohmann::json::array();
    for (const auto& h : histories) {
        std::string start_snapshot, end_snapshot;
        nlohmann::json speed_json;

        if (h.eventType == 0) { // 불법주정차
            start_snapshot = h.startSnapshot;
            end_snapshot = h.endSnapshot;
            speed_json = nullptr;
        } else if (h.eventType == 1) { // 과속
    start_snapshot = "";
    end_snapshot = "";
    if (h.speed.has_value()) {
        float rounded = std::round(h.speed.value() * 100) / 100.0f;
        speed_json = rounded;
    } else {
        speed_json = nullptr;
    }
} else { // 기타(어린이감지)
            start_snapshot = "";
            end_snapshot = "";
            speed_json = nullptr;
        }

        data.push_back({
            {"id", h.id},
            {"date", h.date},
            {"image_path", h.imagePath},
            {"plate_number", h.plateNumber},
            {"event_type", h.eventType},
            {"start_snapshot", start_snapshot},
            {"end_snapshot", end_snapshot},
            {"speed", speed_json}
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

    nlohmann::json data = nlohmann::json::array();
    for (const auto& h : histories) {
        if (h.eventType == 0) { // 불법주정차
            data.push_back({
                {"id", h.id},
                {"date", h.date},
                {"image_path", h.imagePath},
                {"plate_number", h.plateNumber},
                {"event_type", h.eventType},
                {"start_snapshot", h.startSnapshot},
                {"end_snapshot", h.endSnapshot}
                // speed 없음
            });
        } else if (h.eventType == 1) { // 과속
            nlohmann::json obj = {
                {"id", h.id},
                {"date", h.date},
                {"image_path", h.imagePath},
                {"plate_number", h.plateNumber},
                {"event_type", h.eventType}
                // start_snapshot, end_snapshot 없음
            };
            if (h.speed.has_value()) {
                float rounded = std::round(h.speed.value() * 100) / 100.0f;
                obj["speed"] = rounded;
            }
            data.push_back(obj);
        } else if (h.eventType == 2) { // 어린이 감지
            data.push_back({
                {"id", h.id},
                {"date", h.date},
                {"image_path", h.imagePath},
                {"event_type", h.eventType}
                // plate_number, speed, start_snapshot, end_snapshot 없음
            });
        }
    }

    nlohmann::json response = {
        {"status", "success"},
        {"code", 200},
        {"message", "History retrieved successfully"},
        {"data", data}
    };

    return response.dump();
}





std::string CommandHandler::handleGetImage(const std::string& payload) {
    std::string imagePath = payload;

    if (imagePath.empty()) {
        return R"({"status": "error", "code": 400, "message": "Image path is missing"})";
    }

    // 실제 파일이 존재하는지 확인
    if (!std::filesystem::exists(imagePath)) {
        return R"({"status": "error", "code": 404, "message": "Image not found"})";
    }

    nlohmann::json response = {
        {"status", "success"},
        {"code", 200},
        {"message", "Image path resolved successfully"},
        {"image_path", imagePath}
    };

    return response.dump();
}

std::string CommandHandler::handleChangeFrame(const std::string& payload) {
    std::istringstream iss(payload);
    int menu_type;
    int bool_val;

    iss >> menu_type >> bool_val;

    if (iss.fail() || (menu_type != 0 && menu_type != 1) || (bool_val != 0 && bool_val != 1)) {
        return R"({"status": "error", "code": 400, "message": "Invalid input format"})";
    }

    const std::string config_path = "/dev/shm/overlay_config";

    // 파일 읽기
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        return R"({"status": "error", "code": 500, "message": "Failed to open overlay_config"})";
    }
    nlohmann::json config;
    try {
        ifs >> config;
    } catch (...) {
        return R"({"status": "error", "code": 500, "message": "Failed to parse overlay_config"})";
    }

    // menu_type에 따라 필드 선택
    if (menu_type == 0) {
        config["show_bbox"] = static_cast<bool>(bool_val);
    } else if (menu_type == 1) {
        config["show_timestamp"] = static_cast<bool>(bool_val);
    }

    // 다시 저장
    std::ofstream ofs(config_path);
    if (!ofs.is_open()) {
        return R"({"status": "error", "code": 500, "message": "Failed to write overlay_config"})";
    }
    ofs << config.dump();
    ofs.close();

    return R"({"status": "success", "code": 200, "message": "Overlay config updated"})";
}

std::string CommandHandler::handleGetFrame(const std::string& payload) {
    const std::string config_path = "/dev/shm/overlay_config";
    nlohmann::json config;

    // 파일 읽기
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        return R"({"status": "error", "code": 500, "message": "Failed to open overlay_config"})";
    }
    try {
        ifs >> config;
    } catch (...) {
        return R"({"status": "error", "code": 500, "message": "Failed to parse overlay_config"})";
    }

    nlohmann::json response = {
        {"status", "success"},
        {"code", 200},
        {"message", "Overlay config retrieved"},
        {"data", config}
    };
    return response.dump();
}
