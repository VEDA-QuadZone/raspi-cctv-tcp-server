// include/db/DBManager.hpp

#ifndef DBMANAGER_HPP
#define DBMANAGER_HPP

#include <string>
#include <sqlite3.h>

class DBManager {
public:
    DBManager(const std::string& dbPath);
    ~DBManager();

    bool open();                           // DB 열기
    void close();                          // DB 닫기
    bool isOpen() const;                   // DB가 열려있는지 확인
    bool execute(const std::string& query); // SQL 쿼리 실행
    sqlite3* getDB() const;

private:
    std::string dbPath_;
    sqlite3* db_;
    bool isOpen_;
};

#endif // DBMANAGER_HPP
