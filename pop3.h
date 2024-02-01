#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "ws2_32.lib")

#define POP3_SSL_PORT "995" // POP3_SSL端口
#define BUFFER_SIZE 10240

using namespace std;

class POP3_SSL {
private:
    SOCKET pop3_socket;
    SSL* ssl_socket;
    SSL_CTX* ssl_ctx;
    char buffer[BUFFER_SIZE + 1];

public:
    POP3_SSL() {
        SSL_library_init();
        SSL_load_error_strings();
        memset(buffer, 0, BUFFER_SIZE);
    }

    ~POP3_SSL() {
        cleanup();
    }

    void cleanup() {
        if (ssl_socket != nullptr) {
            SSL_shutdown(ssl_socket);
            SSL_free(ssl_socket);
            ssl_socket = nullptr;
        }
        if (pop3_socket != INVALID_SOCKET) {
            closesocket(pop3_socket);
            pop3_socket = INVALID_SOCKET;
        }
        WSACleanup();
    }

    bool initWSA();
    bool getServerInfo(const std::string& server_url, addrinfo** server_info);
    bool createSocket(SOCKET& pop3_socket, addrinfo* server_info);
    bool connectServer(SOCKET pop3_socket, addrinfo* server_info);
    bool initSSL(SSL_CTX*& ssl_ctx);
    bool setupSSL(SSL** ssl_socket, SOCKET pop3_socket, SSL_CTX* ssl_ctx);
    bool connect_to_pop3_server(const std::string& server_url);
    bool login_to_pop3_server(const std::string& username, const std::string& password);
    vector<string> retrieve_emails();
    string retrieve_email_content(int message_number);
    void retrieve_and_print_email_info(POP3_SSL& pop3_ssl_client);
    void save_email_content(POP3_SSL& pop3_ssl_client, int message_number);
    void extract_email_content(int message_number, const string& response);
    bool delete_email(int message_number);
    bool close_connection();
};



