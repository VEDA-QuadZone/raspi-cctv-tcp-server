// =====================
// include/server/CommandHandler.hpp
// =====================
#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <string>
#include <sqlite3.h>
#include <openssl/ssl.h>
#include "../db/repository/UserRepository.hpp"
#include "../db/repository/HistoryRepository.hpp"
#include "./ImageHandler.hpp"

class CommandHandler {
public:
    CommandHandler(sqlite3* db, ImageHandler* ih);

    std::string handle(const std::string& commandStr);
    void handleGetImage(SSL* ssl, const std::string& imagePath);

private:
    UserRepository userRepo;
    HistoryRepository historyRepo;
    ImageHandler* imageHandler_;

    std::string handleRegister(const std::string& payload);
    std::string handleLogin(const std::string& payload);
    std::string handleResetPassword(const std::string& payload);
    std::string handleGetHistory(const std::string& payload);
    std::string handleAddHistory(const std::string& payload);
    std::string handleGetHistoryByEventType(const std::string& payload);
    std::string handleGetHistoryByDateRange(const std::string& payload);
    std::string handleGetHistoryByEventTypeAndDateRange(const std::string& payload);
    std::string handleChangeFrame(const std::string& payload);
    std::string handleGetFrame(const std::string& payload);
    std::string handleGetLog(const std::string& payload);
};

#endif // COMMAND_HANDLER_HPP