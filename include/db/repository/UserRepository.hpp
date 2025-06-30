#ifndef USER_REPOSITORY_HPP
#define USER_REPOSITORY_HPP

#include <string>
#include <optional>
#include "../model/User.hpp"
#include "sqlite3.h"

class UserRepository {
public:
    explicit UserRepository(sqlite3* db);

    bool createUser(const User& user);
    std::optional<User> getUserByEmail(const std::string& email);
    std::optional<User> getUserById(int id);
    bool updateUserPassword(int id, const std::string& newPasswordHash);
    bool deleteUser(int id);

private:
    sqlite3* db;
};

#endif // USER_REPOSITORY_HPP
