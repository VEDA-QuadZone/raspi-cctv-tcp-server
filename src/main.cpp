#include "../../include/server/TcpServer.hpp"
#include "../../include/server/CommandHandler.hpp"
#include "../../include/db/DBManager.hpp"
#include "../../include/db/DBInitializer.hpp"

#include <iostream>
#include <sqlite3.h>

// 전역 포인터 선언
CommandHandler* commandHandler = nullptr;

int main() {
    // 1. DB 연결
    DBManager db("server_data.db");  // 파일로 저장할 경우
    if (!db.open()) {
        std::cerr << "Failed to open DB" << std::endl;
        return 1;
    }

    // 2. 테이블 생성
    DBInitializer::init(db);

    // 3. CommandHandler 인스턴스 생성 및 전역 포인터 설정
    TcpServer server(db.getDB());
    server.setupSocket(8080);
    server.start();

    return 0;
}