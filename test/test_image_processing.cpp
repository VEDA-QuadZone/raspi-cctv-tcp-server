#include "../include/db/repository/HistoryRepository.hpp"
#include "../include/db/model/History.hpp"
#include <sqlite3.h>
#include <iostream>
#include <memory>

// DB 경로 (필요 시 실제 DB 경로에 맞게 수정)
#define DB_PATH "../build/server_data.db"
int main() {
    sqlite3* db;
    int rc = sqlite3_open(DB_PATH, &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    // Use the correct namespace if HistoryRepository is inside a namespace, e.g.:
    // mynamespace::HistoryRepository historyRepo(db);
    HistoryRepository historyRepo(db);

    // 더미 데이터 1
    History dummy1 = {
        "2025-07-02 18:30:00",
        "images/image01.jpeg",
        "12가3456",
        0
    };

    // 더미 데이터 2
    History dummy2 = {
        "2025-07-02 18:31:00",
        "images/image02.jpg",
        "34나5678",
        1
    };

    if (historyRepo.createHistory(dummy1)) {
        std::cout << "Inserted dummy1 successfully." << std::endl;
    } else {
        std::cerr << "Failed to insert dummy1." << std::endl;
    }

    if (historyRepo.createHistory(dummy2)) {
        std::cout << "Inserted dummy2 successfully." << std::endl;
    } else {
        std::cerr << "Failed to insert dummy2." << std::endl;
    }

    sqlite3_close(db);
    return 0;
}
