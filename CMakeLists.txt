cmake_minimum_required(VERSION 3.10)
project(raspi-cctv-server)

set(CMAKE_CXX_STANDARD 17)

# include 디렉토리 추가
include_directories(
    include
    include/lib
)

# 소스 파일 목록
set(SOURCES
    src/main.cpp
    src/db/DBManager.cpp
    src/db/DBInitializer.cpp
    src/db/UserRepository.cpp
    src/db/HistoryRepository.cpp
    src/server/TcpServer.cpp
)

# 실행 파일 생성
add_executable(server ${SOURCES})

# SQLite3, OpenSSL 라이브러리 링크
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
target_link_libraries(server SQLite::SQLite3 OpenSSL::SSL)




# 테스트용 실행 파일
set(SOURCES_TEST
    test/test_tcpserver.cpp
    src/server/TcpServer.cpp
    src/db/DBManager.cpp
    src/db/DBInitializer.cpp
    src/db/UserRepository.cpp
    src/db/HistoryRepository.cpp
)

add_executable(tcpserver-test ${SOURCES_TEST})
target_link_libraries(tcpserver-test SQLite::SQLite3)
