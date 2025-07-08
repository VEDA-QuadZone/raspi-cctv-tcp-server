#include "../../include/db/repository/UserRepository.hpp"
#include <iostream>

UserRepository::UserRepository(sqlite3* db) : db(db) {}

// 1. 사용자 생성
bool UserRepository::createUser(const User& user) {
    const char* sql = "INSERT INTO users (email, password_hash, created_at) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 바인딩
    sqlite3_bind_text(stmt, 1, user.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.passwordHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.createdAt.c_str(), -1, SQLITE_TRANSIENT);

    // 실행
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

// 2. 사용자 조회 (Email)
std::optional<User> UserRepository::getUserByEmail(const std::string& email) {
    const char* sql = "SELECT id, email, password_hash, created_at FROM users WHERE email = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare SELECT statement: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        sqlite3_finalize(stmt);
        return user;
    } else {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
}

// 3. 사용자 조회 (ID)
std::optional<User> UserRepository::getUserById(int id) {
    const char* sql = "SELECT id, email, password_hash, created_at FROM users WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare SELECT statement: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        sqlite3_finalize(stmt);
        return user;
    } else {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
}

// 4. 비밀번호 변경
bool UserRepository::updateUserPassword(int id, const std::string& newPasswordHash) {
    const char* sql = "UPDATE users SET password_hash = ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare UPDATE statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 바인딩: 1번 자리에 새 비밀번호, 2번 자리에 사용자 id
    sqlite3_bind_text(stmt, 1, newPasswordHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute UPDATE statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

// 5. 사용자 삭제
bool UserRepository::deleteUser(int id) {
    const char* sql = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare DELETE statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 사용자 ID 바인딩
    sqlite3_bind_int(stmt, 1, id);

    // 쿼리 실행
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute DELETE statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    // 실제 삭제된 행이 없으면 실패로 판단
    int changes = sqlite3_changes(db);
    if (changes == 0) {
        std::cerr << "Delete failed: No user found with ID = " << id << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}
