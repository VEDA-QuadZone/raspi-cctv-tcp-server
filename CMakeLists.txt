cmake_minimum_required(VERSION 3.10)
project(raspi-cctv-server)

set(CMAKE_CXX_STANDARD 17)

# include 디렉토리
include_directories(
  include
  include/lib
)

# 공통 소스
set(COMMON_SOURCES
  src/server/TcpServer.cpp
  src/server/CommandHandler.cpp
  src/server/ImageHandler.cpp
  src/db/DBManager.cpp
  src/db/DBInitializer.cpp
  src/db/UserRepository.cpp
  src/db/HistoryRepository.cpp
  src/util/EndianUtils.cpp
)

# 실행 파일
add_executable(server
  src/main.cpp
  ${COMMON_SOURCES}
)

add_executable(tcpserver-test
  test/test_CommandHandler.cpp
  ${COMMON_SOURCES}
)

# 필요한 패키지
find_package(Threads   REQUIRED)
find_package(SQLite3   REQUIRED)
find_package(OpenSSL   REQUIRED)

# 링크
target_link_libraries(server
  PRIVATE
    Threads::Threads
    SQLite::SQLite3
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_link_libraries(tcpserver-test
  PRIVATE
    Threads::Threads
    SQLite::SQLite3
    OpenSSL::SSL
    OpenSSL::Crypto
)
