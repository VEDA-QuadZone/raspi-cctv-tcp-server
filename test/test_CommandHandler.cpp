#include "../../include/server/CommandHandler.hpp"
#include "../../include/db/DBManager.hpp"
#include "../../include/db/DBInitializer.hpp"
#include "../../include/db/repository/HistoryRepository.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>  // C++17 이상 필요
#include <fstream> // 파일 생성용


int main() {
    // 1. 메모리 DB 연결
    DBManager db(":memory:");
    if (!db.open()) {
        std::cerr << "Failed to open in-memory database" << std::endl;
        return 1;
    }

    DBInitializer::init(db);
    CommandHandler handler(db.getDB());
    HistoryRepository hr(db.getDB());

    // 2. REGISTER
    std::string regRes = handler.handle("REGISTER user@example.com mypassword");
    std::cout << "REGISTER: " << regRes << std::endl;
    assert(regRes.find("success") != std::string::npos);

    // 3. LOGIN
    std::string loginRes = handler.handle("LOGIN user@example.com mypassword");
    std::cout << "LOGIN: " << loginRes << std::endl;
    assert(loginRes.find("success") != std::string::npos);

    // 4. RESET_PASSWORD
    std::string resetRes = handler.handle("RESET_PASSWORD user@example.com newpass");
    std::cout << "RESET_PASSWORD: " << resetRes << std::endl;
    assert(resetRes.find("success") != std::string::npos);

    // 5. LOGIN with new password
    std::string loginAfterReset = handler.handle("LOGIN user@example.com newpass");
    std::cout << "LOGIN after reset: " << loginAfterReset << std::endl;
    assert(loginAfterReset.find("success") != std::string::npos);

    // 6. 히스토리 더미 데이터 삽입
    hr.createHistory({"2025-01-01 12:00:00", "images/img1.jpg", "12가3456", 0});
    hr.createHistory({"2025-01-10 12:00:00", "images/img2.jpg", "34나7890", 1});
    hr.createHistory({"2025-01-20 12:00:00", "images/img3.jpg", "56다1234", 0});

    // 7. GET_HISTORY
    std::string res = handler.handle("GET_HISTORY user@example.com 10 0");
    std::cout << "GET_HISTORY: " << res << std::endl;
    assert(res.find("img1.jpg") != std::string::npos);

    // 8. GET_HISTORY_BY_EVENT_TYPE
    res = handler.handle("GET_HISTORY_BY_EVENT_TYPE user@example.com 0 10 0");
    std::cout << "GET_HISTORY_BY_EVENT_TYPE: " << res << std::endl;
    assert(res.find("img3.jpg") != std::string::npos);

    // 9. GET_HISTORY_BY_DATE_RANGE
    res = handler.handle("GET_HISTORY_BY_DATE_RANGE user@example.com 2025-01-01 2025-01-31 10 0");
    std::cout << "GET_HISTORY_BY_DATE_RANGE: " << res << std::endl;
    assert(res.find("img2.jpg") != std::string::npos);

    // 10. GET_HISTORY_BY_EVENT_TYPE_AND_DATE_RANGE
    res = handler.handle("GET_HISTORY_BY_EVENT_TYPE_AND_DATE_RANGE user@example.com 0 2025-01-01 2025-01-31 10 0");
    std::cout << "GET_HISTORY_BY_EVENT_TYPE_AND_DATE_RANGE: " << res << std::endl;
    assert(res.find("img3.jpg") != std::string::npos);

    // 11. 이미지 더미 파일 생성
    std::filesystem::create_directories("images");
    std::ofstream("images/img1.jpg");  // 빈 파일 생성
    std::ofstream("images/img2.jpg");
    std::ofstream("images/img3.jpg");

    // 12. GET_IMAGE
    res = handler.handle("GET_IMAGE images/img1.jpg");
    std::cout << "GET_IMAGE: " << res << std::endl;
    assert(res.find("images/img1.jpg") != std::string::npos);

    db.close();
    std::cout << "All CommandHandler tests passed!" << std::endl;
    return 0;
}
