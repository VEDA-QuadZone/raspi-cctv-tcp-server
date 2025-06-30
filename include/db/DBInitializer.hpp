#ifndef DB_INITIALIZER_HPP
#define DB_INITIALIZER_HPP

#include "DBManager.hpp"

class DBInitializer {
public:
    static void init(DBManager& db);  // 테이블 생성 수행
};

#endif
