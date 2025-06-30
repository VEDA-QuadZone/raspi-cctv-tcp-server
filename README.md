# raspi-cctv-server


라즈베리파이 기반의 스쿨존 모니터링 시스템 서버

## 프로젝트 개요

`raspi-cctv-server`는 라즈베리파이에 연결된 CCTV로부터 검출된 차량 정보를 기록하고, 사용자 인증 및 히스토리 조회 기능을 제공하는 **TCP 기반 서버**입니다. 
실시간 스트리밍은 RTSP를 통해 외부에서 처리되며, 서버는 차량 번호판, 이벤트 유형, 이미지 경로 등을 DB에 저장하고 클라이언트 요청에 응답합니다.

## 기술 스택

- **언어**: C++
- **네트워크**: TCP Socket Server
- **보안**: OpenSSL (예정)
- **DB**: SQLite3
- **OS**: Raspberry Pi OS (Linux 기반)
- **라이브러리**:
  - `sqlite3`
  - `OpenSSL` 
  - `pthread` 

## 🧩 주요 기능

- 회원가입, 로그인, 로그아웃
- 비밀번호 재설정 (SHA256 해싱)
- 차량 검출 정보 저장 및 조회
- 날짜/이벤트 필터 기반 검색
- 세션 유지 관리
- 이미지 경로 기반 파일 요청

## 🗂️ 디렉토리 구조

```

tcp-server-backend/
├── CMakeLists.txt # CMake 빌드 설정
├── README.md # 프로젝트 설명
├── include/ # 헤더 파일 디렉토리
│ ├── server/ # 서버 관련 클래스 헤더 (TCPServer 등)
│ ├── db/ # DBManager, 모델 관련 헤더
│ ├── session/ # 세션 처리 관련 헤더
│ └── utils/ # 해싱, JSON 파싱 등의 유틸리티 헤더
├── src/ # 소스 파일 디렉토리
│ ├── main.cpp # 프로그램 진입점
│ ├── server/ # 서버 처리 로직
│ ├── db/ # DB 연동 처리
│ ├── session/ # 세션 유지 및 토큰 관리
│ └── utils/ # 공통 함수 및 도구
├── assets/ # 기본 이미지 또는 기타 정적 리소스
├── test/ # 유닛 테스트 코드
├── scripts/ # 빌드/초기화/배포 스크립트
├── .env # 환경 변수 설정 파일

````

## 빌드 및 실행

```bash
mkdir build && cd build
cmake ..
make
./raspi-cctv-server
````

## ⚙️ API/명령 프로토콜

서버는 **TCP 문자열 명령 프로토콜** 기반으로 클라이언트와 통신합니다.

예시 요청:

```
LOGIN email=user@example.com&password=1234
```

예시 응답:

```
200 OK
session=abc123def456
```
