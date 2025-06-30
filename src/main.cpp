#include <iostream>
#include <vector>
#include "../include/db/DBManager.hpp"
#include "../include/db/DBInitializer.hpp"
#include "../include/db/repository/HistoryRepository.hpp"

int main() {
    // 1. DB 연결
    DBManager dbManager("cctv.db");
    if (!dbManager.open()) {
        std::cerr << "[main] Failed to open database." << std::endl;
        return 1;
    }

    // 2. 테이블 초기화 (users, history)
    DBInitializer::init(dbManager);

    // 3. HistoryRepository 생성
    sqlite3* dbHandle = dbManager.getDB();
    HistoryRepository historyRepo(dbHandle);

    // 4. 테스트용 history 데이터 삽입
    std::vector<History> testHistories = {
        {-1, "2025-06-28 10:00:00", "/images/001.jpg", "12가3456", 0},
        {-1, "2025-06-29 11:15:00", "/images/002.jpg", "34나7890", 1},
        {-1, "2025-06-30 12:30:00", "/images/003.jpg", "56다1234", 1}
    };

    for (const auto& h : testHistories) {
        if (historyRepo.createHistory(h)) {
            std::cout << "[main] Inserted history: " << h.imagePath << std::endl;
        }
    }

    // 5. 날짜 + 이벤트 타입 필터 테스트
    std::string startDate = "2025-06-28 00:00:00";
    std::string endDate = "2025-06-30 23:59:59";
    int eventType = 1;
    int limit = 10;
    int offset = 0;

    std::vector<History> filtered = historyRepo.getHistoriesByEventTypeAndDateRange(
        eventType, startDate, endDate, limit, offset
    );

    std::cout << "\n[main] Filtered histories (eventType = 1):\n";
    for (const auto& h : filtered) {
        std::cout << "ID: " << h.id
                  << ", Date: " << h.date
                  << ", Plate: " << h.plateNumber
                  << ", Path: " << h.imagePath
                  << ", Type: " << h.eventType << std::endl;
    }

    // 6. 특정 ID 삭제 테스트
    if (!filtered.empty()) {
        int deleteId = filtered.front().id;
        if (historyRepo.deleteHistory(deleteId)) {
            std::cout << "\n[main] Deleted history with ID: " << deleteId << std::endl;
        }
    }

    dbManager.close();
    return 0;
}