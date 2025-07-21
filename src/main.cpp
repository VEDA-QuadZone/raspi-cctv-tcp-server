#include <signal.h> // ← 꼭 필요!
#include "../../include/server/TcpServer.hpp"
#include "../../include/server/CommandHandler.hpp"
#include "../../include/server/ImageHandler.hpp"
#include "../../include/db/DBManager.hpp"
#include "../../include/db/DBInitializer.hpp"

#include <iostream>

// 전역 포인터 선언
CommandHandler* commandHandler = nullptr;

int main() {
    // 여기에 추가! (가장 상단에)
    signal(SIGPIPE, SIG_IGN);
    
    // 1. DB 연결
    DBManager db("server_data.db");  // 파일로 저장할 경우
    if (!db.open()) {
        std::cerr << "Failed to open DB" << std::endl;
        return 1;
    }

    // 2. 테이블 생성
    DBInitializer::init(db);

    // 3-1. CommandHandler 인스턴스 생성 및 전역 포인터 설정
    CommandHandler handler(db.getDB());
    commandHandler = &handler;

    // 3-2. ImageHandler 인스턴스 생성
    ImageHandler imageHandler(db.getDB()); 

    // 4. TcpServer 실행
    TcpServer server;
    server.setImageHandler(&imageHandler);  // ImageHandler 설정
    server.setupSocket(8080);  // 원하는 포트 번호
    server.start();

    return 0;
}