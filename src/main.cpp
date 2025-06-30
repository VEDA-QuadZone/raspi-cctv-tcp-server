#include "../include/db/DBManager.hpp"
#include <string>

using namespace std;

int main() {
    DBManager db("test.db");

    if (!db.open()) return 1;

    string createQuery = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT);";
    if (!db.execute(createQuery)) return 1;

    db.close();
    return 0;
}
