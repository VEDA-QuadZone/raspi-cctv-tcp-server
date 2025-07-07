#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string>
#include <sqlite3.h>
#include "../db/repository/UserRepository.hpp"
#include "../db/repository/HistoryRepository.hpp"

class CommandHandler {
public:
    CommandHandler(sqlite3* db);

    // 클라이언트로부터 받은 명령어 문자열을 처리하고 응답 문자열을 반환
    // client_fd는 클라이언트 소켓 파일 디스크립터로, 이미지 요청 시 사용됨
    std::string handle(const std::string& commandStr, int client_fd);

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
    bool handleGetImage(int client_fd, const std::string& payload); // 클라이언트 소켓을 통해 이미지 전송

    // 유틸 함수: JSON 파싱, 응답 생성 등
};

#endif // COMMAND_HANDLER_HPP