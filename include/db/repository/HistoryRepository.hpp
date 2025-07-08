#ifndef HISTORY_REPOSITORY_HPP
#define HISTORY_REPOSITORY_HPP

#include <vector>
#include <string>
#include <sqlite3.h>
#include "../model/History.hpp"

class HistoryRepository {
public:
    explicit HistoryRepository(sqlite3* db);

    // 히스토리 생성
    bool createHistory(const History& history);

    // 페이지네이션 적용한 히스토리 조회 (limit & offset)
    std::vector<History> getHistories(int limit, int offset);

    // 이벤트 타입 필터 + 페이지네이션
    std::vector<History> getHistoriesByEventType(int eventType, int limit, int offset);

    // 날짜 범위 필터 + 페이지네이션
    std::vector<History> getHistoriesByDateRange(const std::string& startDate, const std::string& endDate, int limit, int offset);

    // 이벤트 유형 + 날짜 범위 필터 + 페이지네이션
    std::vector<History> getHistoriesByEventTypeAndDateRange(int eventType, const std::string& startDate, const std::string& endDate, int limit, int offset);

    // 특정 ID의 히스토리 삭제
    bool deleteHistory(int id);

private:
    sqlite3* db;
};

#endif // HISTORY_REPOSITORY_HPP
