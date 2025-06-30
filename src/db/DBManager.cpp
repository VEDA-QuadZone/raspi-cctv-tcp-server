#include "../../include/db/DBManager.hpp"
#include <iostream>

using namespace std;

DBManager::DBManager(const string& dbPath)
    : dbPath_(dbPath), db_(nullptr), isOpen_(false) {}

DBManager::~DBManager() {
    close(); // 객체 소멸 시 DB 연결 닫기
}

bool DBManager::open() {
    int result = sqlite3_open(dbPath_.c_str(), &db_);
    if (result == SQLITE_OK) {
        isOpen_ = true;
        return true;
    } else {
        cerr << "[DBManager] Failed to open database: " << sqlite3_errmsg(db_) << endl;
        return false;
    }
}

void DBManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        isOpen_ = false;
    }
}

bool DBManager::isOpen() const {
    return isOpen_;
}

bool DBManager::execute(const string& query) {
    if (!isOpen_) {
        cerr << "[DBManager] Database is not open." << endl;
        return false;
    }

    char* errMsg = nullptr;
    int result = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        cerr << "[DBManager] SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

sqlite3* DBManager::getDB() const {
    return db_;
}