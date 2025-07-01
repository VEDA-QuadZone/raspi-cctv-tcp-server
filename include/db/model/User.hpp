#ifndef USER_HPP
#define USER_HPP

#include <string>

struct User {
    int id = -1; // -1: 초기화되지 않은 상태
    std::string email;
    std::string passwordHash;
    std::string createdAt; // ISO 8601 형식: "2025-06-30T15:00:00"
};

#endif // USER_HPP
