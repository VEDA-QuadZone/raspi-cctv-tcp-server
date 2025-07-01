#include "../../include/server/CommandHandler.hpp"
#include <sstream>
#include <iostream>

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