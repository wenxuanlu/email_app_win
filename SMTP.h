#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#define SMTP_PORT 25    // SMTP协议的熟知端口号
#define BUFFER_LENTH 1024


using namespace std;

class SMTP{
private:
	SOCKET smtp_socket;
	char buffer[BUFFER_LENTH];
	size_t buffer_len = sizeof(buffer);

	string base64_encode(string to_encode);
	vector<string> get_url_username(string sender_addr);
	bool connect_smtp_server(string server_url);
	void close();
	void send_EHLO(string server_url);
	void send_AUTH_LOGIN(string server_url);
	bool send_username_password(string server_url, string user_addr, string username, string password);
	void send_receiver_addr(string server_url, vector<string> receiver_addr);
	void send_data(string server_url, string sender_addr, vector<string> receiver_addr, string subject, string data);

public:
	string send_email(string sender_addr, string sender_password, vector<string> receiver_addr, string subject, string data);
};

