cmake_minimum_required(VERSION 3.10)
project(raspi-cctv-tcp-server)

set(CMAKE_CXX_STANDARD 17)

# include 디렉토리 추가
include_directories(
    include
    include/lib
)

# 공통 소스 (서버 & 테스트에서 공통으로 사용)
set(COMMON_SOURCES
    src/server/TcpServer.cpp
    src/server/CommandHandler.cpp
    src/db/DBManager.cpp
    src/db/DBInitializer.cpp
    src/db/UserRepository.cpp
    src/db/HistoryRepository.cpp
)

# 메인 실행 파일 (server)
add_executable(server
    src/main.cpp
    ${COMMON_SOURCES}
)

# SQLite3, OpenSSL 찾기
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

# 링크 라이브러리
target_link_libraries(server SQLite::SQLite3 OpenSSL::SSL)

# 테스트 실행 파일 (tcpserver-test)
add_executable(tcpserver-test
    test/test_image_processing.cpp
    ${COMMON_SOURCES}
)

target_link_libraries(tcpserver-test SQLite::SQLite3 OpenSSL::SSL)