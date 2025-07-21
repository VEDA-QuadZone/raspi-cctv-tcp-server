#include "../../include/db/DBInitializer.hpp"
#include <iostream>

using namespace std;

void DBInitializer::init(DBManager& db) {
    string createUserTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";

    string createHistoryTable = R"(
        CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY,
            date DATETIME NOT NULL,
            image_path TEXT NOT NULL,
            plate_number TEXT NOT NULL,
            event_type INTEGER NOT NULL,
            start_snapshot TEXT,
            end_snapshot TEXT,
            speed FLOAT
        );
    )";

    if (!db.execute(createUserTable)) {
        cerr << "[DBInitializer] Failed to create 'users' table." << endl;
    }

    if (!db.execute(createHistoryTable)) {
        cerr << "[DBInitializer] Failed to create 'history' table." << endl;
    }
}
