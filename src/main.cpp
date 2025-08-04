#include <signal.h>           // SIGPIPE 무시용
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "server/TcpServer.hpp"
#include "server/CommandHandler.hpp"
#include "server/ImageHandler.hpp"
#include "db/DBManager.hpp"
#include "db/DBInitializer.hpp"

#include <iostream>

// 전역 포인터 선언 (handleClientSSL 에서 사용)
CommandHandler* commandHandler = nullptr;

int main() {
    // SIGPIPE 방지
    signal(SIGPIPE, SIG_IGN);

    // ─── OpenSSL 초기화 ────────────────────────────────
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* sslCtx = SSL_CTX_new(method);
    if (!sslCtx) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    // 인증서·키 로드
    if (SSL_CTX_use_certificate_file(sslCtx,
            "/home/sejin/myCA/tcp_server/certs/tcp_server.cert.pem",
            SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    if (SSL_CTX_use_PrivateKey_file(sslCtx,
            "/home/sejin/myCA/tcp_server/private/tcp_server.key.pem",
            SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    if (!SSL_CTX_check_private_key(sslCtx)) {
        std::cerr << "Private key does not match the certificate\n";
        return 1;
    }

    // CA 로드 (클라이언트 인증서 검증용)
    if (!SSL_CTX_load_verify_locations(sslCtx,
            "/home/sejin/myCA/certs/ca.cert.pem", nullptr)) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    // 검증 옵션 설정
    SSL_CTX_set_verify(sslCtx,
        SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
        nullptr);
    SSL_CTX_set_verify_depth(sslCtx, 4);
    SSL_CTX_set_options(sslCtx,
        SSL_OP_NO_SSLv2 |
        SSL_OP_NO_SSLv3 |
        SSL_OP_NO_COMPRESSION |
        SSL_OP_CIPHER_SERVER_PREFERENCE
    );
    // ────────────────────────────────────────────────────

    // 1. DB 연결
    DBManager db("server_data.db");
    if (!db.open()) {
        std::cerr << "Failed to open DB\n";
        return 1;
    }
    // 2. 테이블 생성
    DBInitializer::init(db);

    // 3. 핸들러 생성
    ImageHandler imageHandler(db.getDB());                  // 먼저 생성
CommandHandler handler(db.getDB(), &imageHandler);      // ✅ 인자 2개 전달
commandHandler = &handler;

    // 4. 서버 실행
    TcpServer server(sslCtx);
    server.setImageHandler(&imageHandler);
    server.setupSocket(8080);
    server.start();

    SSL_CTX_free(sslCtx);
    return 0;
}
