cmake_minimum_required(VERSION 3.10)
project(raspi-cctv-server)

set(CMAKE_CXX_STANDARD 17)

# include 디렉토리 추가
include_directories(include)

# 소스 파일 목록
set(SOURCES
    src/main.cpp
    src/db/DBManager.cpp
    src/db/DBInitializer.cpp
    src/db/UserRepository.cpp
)

# 실행 파일 생성
add_executable(server ${SOURCES})

# SQLite3 라이브러리 링크
find_package(SQLite3 REQUIRED)
target_link_libraries(server SQLite::SQLite3)