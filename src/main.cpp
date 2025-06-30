#include "../include/db/DBManager.hpp"
#include "../include/db/DBInitializer.hpp"
#include <string>

using namespace std;

int main() {
    DBManager db("test.db");

    if (!db.open()) return 1;

    DBInitializer::init(db); // 테이블 생성

    db.close();
    return 0;
}
