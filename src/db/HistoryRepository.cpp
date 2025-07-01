#include "../../include/db/repository/HistoryRepository.hpp"
#include <iostream>

HistoryRepository::HistoryRepository(sqlite3* db) : db(db) {}

// 히스토리 생성
bool HistoryRepository::createHistory(const History& history) {
    const char* sql = "INSERT INTO history (date, image_path, plate_number, event_type) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare INSERT statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, history.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, history.imagePath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, history.plateNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, history.eventType);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute INSERT: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

// 페이지네이션 적용 전체 조회
std::vector<History> HistoryRepository::getHistories(int limit, int offset) {
    std::vector<History> histories;
    const char* sql = "SELECT id, date, image_path, plate_number, event_type FROM history ORDER BY date DESC LIMIT ? OFFSET ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return histories;
    }

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        History history;
        history.id = sqlite3_column_int(stmt, 0);
        history.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        history.imagePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        history.plateNumber = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        history.eventType = sqlite3_column_int(stmt, 4);

        histories.push_back(history);
    }

    sqlite3_finalize(stmt);
    return histories;
}

// 이벤트 유형 필터 + 페이지네이션
std::vector<History> HistoryRepository::getHistoriesByEventType(int eventType, int limit, int offset) {
    std::vector<History> histories;
    const char* sql = "SELECT id, date, image_path, plate_number, event_type "
                      "FROM history WHERE event_type = ? "
                      "ORDER BY date DESC LIMIT ? OFFSET ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return histories;
    }

    sqlite3_bind_int(stmt, 1, eventType);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        History history;
        history.id = sqlite3_column_int(stmt, 0);
        history.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        history.imagePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        history.plateNumber = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        history.eventType = sqlite3_column_int(stmt, 4);

        histories.push_back(history);
    }

    sqlite3_finalize(stmt);
    return histories;
}

// 날짜 범위 필터 + 페이지네이션
std::vector<History> HistoryRepository::getHistoriesByDateRange(
    const std::string& startDate,
    const std::string& endDate,
    int limit,
    int offset
) {
    std::vector<History> histories;
    const char* sql =
        "SELECT id, date, image_path, plate_number, event_type "
        "FROM history "
        "WHERE date BETWEEN ? AND ? "
        "ORDER BY date DESC LIMIT ? OFFSET ?;";
    
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return histories;
    }

    sqlite3_bind_text(stmt, 1, startDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, endDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, limit);
    sqlite3_bind_int(stmt, 4, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        History history;
        history.id = sqlite3_column_int(stmt, 0);
        history.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        history.imagePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        history.plateNumber = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        history.eventType = sqlite3_column_int(stmt, 4);

        histories.push_back(history);
    }

    sqlite3_finalize(stmt);
    return histories;
}

// 이벤트 유형 + 날짜 범위 필터 + 페이지네이션
std::vector<History> HistoryRepository::getHistoriesByEventTypeAndDateRange(
    int eventType,
    const std::string& startDate,
    const std::string& endDate,
    int limit,
    int offset
) {
    std::vector<History> histories;
    const char* sql =
        "SELECT id, date, image_path, plate_number, event_type "
        "FROM history "
        "WHERE event_type = ? AND date BETWEEN ? AND ? "
        "ORDER BY date DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return histories;
    }

    sqlite3_bind_int(stmt, 1, eventType);
    sqlite3_bind_text(stmt, 2, startDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, endDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, limit);
    sqlite3_bind_int(stmt, 5, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        History history;
        history.id = sqlite3_column_int(stmt, 0);
        history.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        history.imagePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        history.plateNumber = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        history.eventType = sqlite3_column_int(stmt, 4);

        histories.push_back(history);
    }

    sqlite3_finalize(stmt);
    return histories;
}

// 히스토리 삭제
bool HistoryRepository::deleteHistory(int id) {
    const char* sql = "DELETE FROM history WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return false;
    }

    // 실제로 삭제된 row가 있는지 확인 (0이면 해당 ID가 없던 것)
    return sqlite3_changes(db) > 0;
}

