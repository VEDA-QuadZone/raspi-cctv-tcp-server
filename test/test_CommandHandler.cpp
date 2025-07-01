#include "../../include/server/CommandHandler.hpp"
#include "../../include/db/DBManager.hpp"
#include "../../include/db/DBInitializer.hpp"
#include <cassert>
#include <iostream>

int main() {
    DBManager db(":memory:"); // 메모리 DB 사용
    if (!db.open()) {
        std::cerr << "Failed to open in-memory database" << std::endl;
        return 1;
    }

    DBInitializer::init(db); // 테이블 생성

    CommandHandler handler(db.getDB());

    // 1. REGISTER
    std::string regRes = handler.handle("REGISTER user@example.com mypassword");
    std::cout << "REGISTER: " << regRes << std::endl;
    assert(regRes.find("success") != std::string::npos);

    // 2. LOGIN
    std::string loginRes = handler.handle("LOGIN user@example.com mypassword");
    std::cout << "LOGIN: " << loginRes << std::endl;
    assert(loginRes.find("success") != std::string::npos);

    // 3. RESET_PASSWORD
    std::string resetRes = handler.handle("RESET_PASSWORD user@example.com newpass");
    std::cout << "RESET_PASSWORD: " << resetRes << std::endl;
    assert(resetRes.find("success") != std::string::npos);

    // 4. LOGIN with new password
    std::string loginAfterReset = handler.handle("LOGIN user@example.com newpass");
    std::cout << "LOGIN after reset: " << loginAfterReset << std::endl;
    assert(loginAfterReset.find("success") != std::string::npos);

    db.close();
    std::cout << "All CommandHandler tests passed!" << std::endl;
    return 0;
}
