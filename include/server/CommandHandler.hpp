#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string>
#include <sqlite3.h>
#include "../db/repository/UserRepository.hpp"
#include "../db/repository/HistoryRepository.hpp"

class CommandHandler {
public:
    CommandHandler(sqlite3* db);

    // 요청 처리 함수: 클라이언트가 보낸 command string을 받아서 응답 string 리턴
    std::string handle(const std::string& commandStr);

private:
    UserRepository userRepo;
    HistoryRepository historyRepo;

    // 명령어별 처리 함수
    std::string handleRegister(const std::string& payload);
    std::string handleLogin(const std::string& payload);
    std::string handleResetPassword(const std::string& payload);
    std::string handleGetHistory(const std::string& payload);
    std::string handleGetHistoryByEventType(const std::string& payload);
    std::string handleGetHistoryByDateRange(const std::string& payload);
    std::string handleGetHistoryByEventTypeAndDateRange(const std::string& payload);
    std::string handleGetImage(const std::string& payload); // 이미지 경로 요청

    // 유틸 함수: JSON 파싱, 응답 생성 등
};

#endif // COMMAND_HANDLER_HPP