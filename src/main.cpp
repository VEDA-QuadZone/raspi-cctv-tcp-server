#include <iostream>
#include "../include/db/repository/UserRepository.hpp"
#include "../include/db/model/User.hpp"
#include <sqlite3.h>

int main() {
    sqlite3* db;
    if (sqlite3_open("test.db", &db) != SQLITE_OK) {
        std::cerr << "Failed to open DB: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    const char* createTableSQL =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "email TEXT UNIQUE NOT NULL, "
        "password_hash TEXT NOT NULL, "
        "created_at TEXT NOT NULL);";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Table creation failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return 1;
    }

    UserRepository repo(db);

    User newUser;
    newUser.email = "test@example.com";
    newUser.passwordHash = "hashedpassword";
    newUser.createdAt = "2025-06-30T15:00:00";

    if (repo.createUser(newUser)) {
        std::cout << "User created successfully\n";
    } else {
        std::cout << "User creation failed\n";
    }

    auto foundUser = repo.getUserByEmail("test@example.com");
    if (foundUser) {
        std::cout << "User found: " << foundUser->email << std::endl;
    } else {
        std::cout << "User not found\n";
    }

    if (foundUser && repo.updateUserPassword(foundUser->id, "newhashedpassword")) {
        std::cout << "Password updated successfully\n";
    }

    auto byId = repo.getUserById(foundUser->id);
    if (byId) {
        std::cout << "User by ID: " << byId->email << std::endl;
    }

    if (repo.deleteUser(foundUser->id)) {
        std::cout << "User deleted successfully\n";
    } else {
        std::cout << "User deletion failed\n";
    }

    sqlite3_close(db);
    return 0;
}
