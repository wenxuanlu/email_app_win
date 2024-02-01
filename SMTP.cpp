#include "SMTP.h"
#include <easyx.h>
#include <graphics.h>
#include <string>
#include <regex>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <locale>
#include <conio.h>
#include <Windows.h>
#include <vector>
#include "pop3.h"
#include "base64.h"
#include "DataBase.cpp"

std::string dbname = "email";
std::string user = "lwx";
std::string password0 = "123456";
std::string host = "101.37.32.213"; //���ݿ���������ڱ���
std::string port = "5432";

string SMTP::base64_encode(string to_encode) {
	string base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned char *input = (unsigned char *)(to_encode.c_str());
	int input_length = to_encode.size();
	string encoded;
	unsigned char tmp[4] = { 0 };
	int line_length = 0;

	for (int i = 0; i < (int)(input_length / 3); i++) {
		tmp[1] = *input++;
		tmp[2] = *input++;
		tmp[3] = *input++;
		encoded += base64_table[tmp[1] >> 2];
		encoded += base64_table[((tmp[1] << 4) | (tmp[2] >> 4)) & 0x3F];
		encoded += base64_table[((tmp[2] << 2) | (tmp[3] >> 6)) & 0x3F];
		encoded += base64_table[tmp[3] & 0x3F];
		line_length += 4;
		if (line_length == 76) {
			encoded += "\r\n"; line_length = 0;
		}
	}

	int remain_num = input_length % 3;
	if (remain_num == 1){
		tmp[1] = *input++;
		encoded += base64_table[(tmp[1] & 0xFC) >> 2];
		encoded += base64_table[((tmp[1] & 0x03) << 4)];
		encoded += "==";
	}
	else if (remain_num == 2){
		tmp[1] = *input++;
		tmp[2] = *input++;
		encoded += base64_table[(tmp[1] & 0xFC) >> 2];
		encoded += base64_table[((tmp[1] & 0x03) << 4) | ((tmp[2] & 0xF0) >> 4)];
		encoded += base64_table[((tmp[2] & 0x0F) << 2)];
		encoded += "=";
	}

	return encoded;
}


// ��ȡSMTP��������URL�Լ��û���
// URLΪ���ͷ���emal��ַ�С�@������ַ�������ǰ���smtp.
// �û���Ϊ���ͷ���emal��ַ�С�@��ǰ���ַ�
vector<string> SMTP:: get_url_username(string sender_addr) {
	vector<string> result;
    string temp = "";
	int loc_of_at = sender_addr.find('@');
	if (loc_of_at <= 0 || (loc_of_at >= sender_addr.size() - 1)) {
		result.push_back("Invalid Sender Address");
		return result;
	}
	temp.append("smtp.");
	temp.append(sender_addr.substr(loc_of_at + 1, sender_addr.size() - loc_of_at - 1));
    result.push_back(temp);
    temp = "";
    temp.append(sender_addr.substr(0, loc_of_at));
    result.push_back(temp);
    return result;
}


void SMTP::close() {
    closesocket(SMTP::smtp_socket);
    WSACleanup();
}


// ��������SMTP������
bool SMTP::connect_smtp_server(string server_url) {
    int if_error;
    
    // ��ʼ��Winsock�⣬���ں����׽��ֲ���
	WSADATA wsaData;
    if_error = WSAStartup(MAKEWORD(2, 2), &wsaData);  // ���ɹ�������0
	if (if_error != 0){
        WSACleanup();
        return false;
	}

    // �����׽���
    // AF_INET ��IPv4��ַ��
    // SOCK_STREAM ��TCP
    // 0 ��Ĭ�ϵ�Э��
    SMTP::smtp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (SMTP::smtp_socket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    HOSTENT *host_entry = gethostbyname(server_url.c_str());
    struct sockaddr_in server_addr; // ��������ַ�Ͷ˿�
    server_addr.sin_addr.S_un.S_addr = *((DWORD *)host_entry->h_addr_list[0]); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SMTP_PORT);

    if_error = connect(SMTP::smtp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (if_error == SOCKET_ERROR){ 
        SMTP::close();
        return false; 
    }

    SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';

    return true;
}


void SMTP::send_EHLO(string server_url) {
    string request = "EHLO " + server_url + "\r\n";
    send(SMTP::smtp_socket, request.data(), request.size(), 0);
    SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';
}


void SMTP::send_AUTH_LOGIN(string server_url) {
    string request = "AUTH LOGIN\r\n";
    send(SMTP::smtp_socket, request.data(), request.size(), 0);
    SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';
}


bool SMTP::send_username_password(string server_url, string user_addr, string username, string password) {
    string request = SMTP::base64_encode(username);
    request += "\r\n";
    send(SMTP::smtp_socket, request.data(), request.size(), 0);
    SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';

    request = SMTP::base64_encode(password);
    request += "\r\n";
    send(SMTP::smtp_socket, request.data(), request.size(), 0);
    SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';

    string respond = buffer;
    if (respond.find("235") == string::npos) {
        SMTP::close();
        return false; // ������û�������
    }

    request = "MAIL FROM: ";
    request += "<" + user_addr + ">" + "\r\n";
    send(SMTP::smtp_socket, request.data(), request.size(), 0);
    SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';
    return true;
}


void SMTP::send_receiver_addr(string server_url, vector<string> receiver_addr) {
    string request;
    for (int i = 0; i < receiver_addr.size(); i++) {
        request = "RCPT TO: ";
        request += "<" + receiver_addr[i] + ">" + "\r\n";
        send(SMTP::smtp_socket, request.data(), request.size(), 0);
        SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';
    }
}


void SMTP::send_data(string server_url, string sender_addr, vector<string> receiver_addr, string subject, string data) {
    string request = "DATA\r\n";
    send(SMTP::smtp_socket, request.data(), request.size(), 0);
    SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';
    
    request = "From: <" + sender_addr + ">\r\n";
    request += "To: ";
    for (int i = 0; i < receiver_addr.size(); i++) {
        request += ("<" + receiver_addr[i] + ">\r\n");
    }
    request += "subject: " + subject + "\r\n\r\n";
    request += data;
    request += "\r\n.\r\n";

    send(SMTP::smtp_socket, request.data(), request.size(), 0);
    SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';
}


string SMTP::send_email(string sender_addr, string sender_password, vector<string> receiver_addr, string subject, string data) {
    vector<string> divided_sender_addr;
    bool flag;

    divided_sender_addr = SMTP::get_url_username(sender_addr);
    if (divided_sender_addr[0] == "Invalid Sender Address") {
        return string("Invalid Sender Address");
    }
    flag = SMTP::connect_smtp_server(divided_sender_addr[0]);
    if (!flag) {
        return string("Connection Failed");
    }
    SMTP::send_EHLO(divided_sender_addr[0]);
    SMTP::send_AUTH_LOGIN(divided_sender_addr[0]);
    flag = SMTP::send_username_password(divided_sender_addr[0], sender_addr, divided_sender_addr[1], sender_password);
    if (!flag) {
        return string("Login failed");
    }
    SMTP::send_receiver_addr(divided_sender_addr[0], receiver_addr);
    SMTP::send_data(divided_sender_addr[0], sender_addr, receiver_addr, subject, data);
    
    string request = "QUIT\r\n";
    send(SMTP::smtp_socket, data.data(), data.size(), 0);
    //SMTP::buffer[recv(SMTP::smtp_socket, SMTP::buffer, SMTP::buffer_len, 0)] = '\0';
    
    SMTP::close();

    return string("Send Success");
}

// ��ʼ��Windows Socket API
bool POP3_SSL::initWSA() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	return (result == 0);
}

// ��ȡ��������Ϣ
bool POP3_SSL::getServerInfo(const string& server_url, addrinfo** server_info) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int result = getaddrinfo(server_url.c_str(), POP3_SSL_PORT, &hints, server_info);
	return (result == 0);
}

// �����׽���
bool POP3_SSL::createSocket(SOCKET& pop3_socket, addrinfo* server_info) {
	pop3_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	return (pop3_socket != INVALID_SOCKET);
}

// ���ӷ�����
bool POP3_SSL::connectServer(SOCKET pop3_socket, addrinfo* server_info) {
	int result = connect(pop3_socket, server_info->ai_addr, server_info->ai_addrlen);
	return (result != SOCKET_ERROR);
}

// ��ʼ��SSL
bool POP3_SSL::initSSL(SSL_CTX*& ssl_ctx) {
	ssl_ctx = SSL_CTX_new(TLS_client_method());
	return (ssl_ctx != nullptr);
}

// ����SSL����
bool POP3_SSL::setupSSL(SSL** ssl_socket, SOCKET pop3_socket, SSL_CTX* ssl_ctx) {
	*ssl_socket = SSL_new(ssl_ctx);
	if (!(*ssl_socket)) {
		SSL_CTX_free(ssl_ctx);
		return false;
	}

	if (SSL_set_fd(*ssl_socket, pop3_socket) != 1) {
		SSL_CTX_free(ssl_ctx);
		return false;
	}

	if (SSL_connect(*ssl_socket) != 1) {
		SSL_CTX_free(ssl_ctx);
		return false;
	}

	return true;
}

// ���ӵ�POP3������
bool POP3_SSL::connect_to_pop3_server(const string& server_url) {
	WSADATA wsaData;
	if (!initWSA()) {
		return false;
	}

	addrinfo* server_info;
	if (!getServerInfo(server_url, &server_info)) {
		WSACleanup();
		return false;
	}

	if (!createSocket(pop3_socket, server_info)) {
		freeaddrinfo(server_info);
		WSACleanup();
		return false;
	}

	if (!connectServer(pop3_socket, server_info)) {
		freeaddrinfo(server_info);
		cleanup();
		return false;
	}

	// ��ʼ��SSL
	SSL_CTX* ssl_ctx;
	if (!initSSL(ssl_ctx)) {
		cleanup();
		return false;
	}

	if (!setupSSL(&ssl_socket, pop3_socket, ssl_ctx)) {
		cleanup();
		return false;
	}

	int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // ����������Ļ�ӭ��Ϣ

	// �ͷ�SSL��Դ
	SSL_CTX_free(ssl_ctx);
	ssl_ctx = NULL;

	// �ͷ�getaddrinfo��������ʱΪ�˻�ȡ��������Ϣ��������ڴ�ռ�
	freeaddrinfo(server_info);

	// ���ӳɹ�
	return true;
}

// ��¼��POP3������
bool POP3_SSL::login_to_pop3_server(const string& username, const string& password) {
	// �����֤
	string request = "USER " + username + "\r\n";
	SSL_write(ssl_socket, request.c_str(), request.size()); // �����û���

	int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE); // ��ȡ�û�����Ӧ
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // ��ӡ�û�����Ӧ

	request = "PASS " + password + "\r\n";
	SSL_write(ssl_socket, request.c_str(), request.size()); // ��������

	bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE); // ��ȡ������Ӧ
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // ��ӡ������Ӧ

	// ���ӳɹ������������֤
	return true;
}

// �����ʼ��б�
vector<string> POP3_SSL::retrieve_emails() {
	vector<string> emails;
	string request = "LIST\r\n";
	SSL_write(ssl_socket, request.c_str(), request.size());// ����LIST����

	int bytes_received = 0;
	string response;
	do {
		bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			response += buffer;
		}
	} while (bytes_received > 0 && response.find("\r\n.\r\n") == string::npos); // ѭ����ȡֱ���յ���������Ӧ

	cout << response << endl; // ����������ʼ��б���Ӧ

	// ������Ӧ��ȡ�ʼ���Ϣ
	size_t start = response.find("\r\n");
	while (start != string::npos) {
		size_t end = response.find("\r\n", start + 2); // ������һ�����з�
		if (end != string::npos) {
			string email_info = response.substr(start + 2, end - start - 2);
			emails.push_back(email_info);
		}
		start = end;
	}

	return emails;
}

// �����ض��ʼ�������
string POP3_SSL::retrieve_email_content(int message_number) {
	string retr_command = "RETR " + to_string(message_number) + "\r\n";
	SSL_write(ssl_socket, retr_command.c_str(), retr_command.size());

	string email_content;
	char temp_buffer[BUFFER_SIZE];
	int bytes_received;
	do {
		bytes_received = SSL_read(ssl_socket, temp_buffer, BUFFER_SIZE);
		if (bytes_received > 0) {
			temp_buffer[bytes_received] = '\0';
			email_content += temp_buffer;
		}
	} while (bytes_received > 0 && email_content.find("\r\n.\r\n") == string::npos);

	return email_content;
}



// ��ȡ�ʼ��ı��������
void POP3_SSL::extract_email_content(int message_number, const string& response) {
	// ʹ��������ʽƥ��Subject��date
	regex subjectRegexCN("Subject: =\\?utf-8\\?B\\?(.+)\\?=\r\n", regex_constants::icase);
	regex subjectRegexEN("Subject: (.+)\r\n");
	regex dateRegex("Date: (.+)\r\n");

	cout << "Email:" << left << setw(20) << message_number;

	// research + base64_decode
	smatch matches;
	// subject
	if (regex_search(response, matches, subjectRegexCN)) {
		string subject = matches[1];
		string decoded_subject = base64_decode(subject);
		// wstring str = to_wstring(decoded_subject);
		cout << "Subject: " << left << hex << setw(50) << decoded_subject;
	}
	else if (regex_search(response, matches, subjectRegexEN)) {
		string subject = matches[1];
		cout << "Subject: " << left << setw(50) << subject;
	}

	// date
	if (regex_search(response, matches, dateRegex)) {
		string date = matches[1];
		cout << "Date: " << date << endl;
	}

}

// ���ʼ����ݴ�ŵ��ļ���
void print_email_info_to_file(const string& email_info) {
	// ʹ��ofstream��������ļ���
	ofstream outputFile("email_content.html");
	if (outputFile.is_open()) {
		// ����HTML���ݵ���ʼλ��
		size_t start = email_info.find("<");
		if (start != std::string::npos) {
			// д��HTML����
			outputFile << email_info.substr(start) << std::endl;
		}
		outputFile.close();
		cout << "Email content saved to 'email_content.html'." << endl;
	}
	else {
		cout << "Error: Unable to open file for writing." << endl;
	}
}

void POP3_SSL::retrieve_and_print_email_info(POP3_SSL& pop3_ssl_client) {
	vector<string> emails = pop3_ssl_client.retrieve_emails();
	cout << "Number of emails: " << emails.size() - 1 << endl; // ������һ�еı�����.����һ���ʼ����ʼ�һ


	// ������ļ�������cout�ض����ļ�
	ofstream outputFile("info.txt", ios::binary);
	streambuf* coutBuf = cout.rdbuf();
	cout.rdbuf(outputFile.rdbuf());

	for (int i = 1; i < emails.size(); i++) {
		string email_info = pop3_ssl_client.retrieve_email_content(i);
		pop3_ssl_client.extract_email_content(i, email_info);
	}

	// �ر��ļ������ָ�cout
	outputFile.close();
	cout.rdbuf(coutBuf);

}

void POP3_SSL::save_email_content(POP3_SSL& pop3_ssl_client, int message_number) {
	vector<string> emails = pop3_ssl_client.retrieve_emails();
	if (emails.size() <= 1 || message_number >= emails.size()) {
		cout << "Error: Invalid message number." << endl;
		return;
	}

	string email_content = pop3_ssl_client.retrieve_email_content(message_number);

	regex contentRegex("Content-Transfer-Encoding: base64\r\n\r\n(.*?)\r\n\r\n",
		std::regex_constants::icase);
	smatch matches;
	regex_search(email_content, matches, contentRegex);
	string email_txt_encoded = matches[1];
	string email_txt = base64_decode(email_txt_encoded);

	// Save Email Content to File
	string Filename = "Email" + to_string(message_number) + ".txt";
	ofstream emailContentFile(Filename);
	if (emailContentFile.is_open()) {
		emailContentFile << email_txt;
		emailContentFile.close();
		cout << "Email content saved to " << Filename << endl;
	}
	else {
		cout << "Error: Unable to open file for writing." << endl;
	}
}

// �ر�POP3����������
bool POP3_SSL::close_connection() {
	string quit_command = "QUIT\r\n";
	SSL_write(ssl_socket, quit_command.c_str(), quit_command.size());

	int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // ����˳��������Ӧ

	// �ر�SSL�׽���
	SSL_shutdown(ssl_socket);
	// SSL_free(ssl_socket);

	// �ر��׽���
	closesocket(pop3_socket);

	// ����SSL�⻷��
	//SSL_CTX_free(ssl_ctx);
	//ssl_ctx = NULL;

	return true;
}

// ɾ��ָ���ʼ�
bool POP3_SSL::delete_email(int message_number) {
	string dele_command = "DELE " + to_string(message_number) + "\r\n";
	SSL_write(ssl_socket, dele_command.c_str(), dele_command.size());

	int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // ���ɾ���ʼ�����Ӧ

	// �����Ӧ�����ɾ���ɹ�����true�����򷵻�false
	bool deleted = (strstr(buffer, "+OK") != nullptr);

	return deleted;
}

void button(int x, int y, int w, int h, TCHAR* text) {
    setbkmode(TRANSPARENT);
    setfillcolor(GREEN);
    fillroundrect(x, y, x + w, y + h, 10, 10);

    TCHAR s1[] = L"����";
    settextstyle(30, 0, s1);
    TCHAR s[50] = L"hello";

    int tx = x + (w - textwidth(text)) / 2;
    int ty = y + (h - textheight(text)) / 2;
    outtextxy(tx, ty, text);

}

void button0(int x, int y, int w, int h, TCHAR* text) {
	setbkmode(TRANSPARENT);
	setfillcolor(WHITE);
	fillroundrect(x, y, x + w, y + h, 10, 10);

	TCHAR s1[] = L"����";
	settextstyle(15, 0, s1);
	TCHAR s[50] = L"hello";

	int tx = x + (w - textwidth(text)) / 2;
	int ty = y + (h - textheight(text)) / 2;
	outtextxy(tx, ty, text);

}

string wide_Char_To_Multi_Byte(wchar_t* pWCStrKey)
{
	//��һ�ε���ȷ��ת�����ֽ��ַ����ĳ��ȣ����ڿ��ٿռ�
	int pSize = WideCharToMultiByte(CP_OEMCP, 0, pWCStrKey, wcslen(pWCStrKey), NULL, 0, NULL, NULL);
	char* pCStrKey = new char[pSize + 1];
	//�ڶ��ε��ý�˫�ֽ��ַ���ת���ɵ��ֽ��ַ���
	WideCharToMultiByte(CP_OEMCP, 0, pWCStrKey, wcslen(pWCStrKey), pCStrKey, pSize, NULL, NULL);
	pCStrKey[pSize] = '\0';
	

	//�����Ҫת����string��ֱ�Ӹ�ֵ����
	string pKey = pCStrKey;
	return pKey;
}

struct Email
{
	int id;
	std::string subject;
	std::string date;
};

LPCWSTR str2LPCWSTR(string str) {
	int len = str.length();
	int lenbf = MultiByteToWideChar(CP_ACP, 0, str.c_str(), len, 0, 0);
	wchar_t* buffer = new wchar_t[lenbf];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), len, buffer, sizeof(wchar_t) * lenbf);
	buffer[len] = 0;
	return buffer;
}

class EasyTextBox
{
private:
	int left = 0, top = 0, right = 0, bottom = 0;	// �ؼ�����
	wchar_t* text = NULL;							// �ؼ�����
	size_t maxlen = 0;									// �ı���������ݳ���

public:
	void Create(int x1, int y1, int x2, int y2, int max)
	{
		maxlen = max;
		text = new wchar_t[maxlen];
		text[0] = 0;
		left = x1, top = y1, right = x2, bottom = y2;

		// �����û�����
		Show();
	}

	~EasyTextBox()
	{
		if (text != NULL)
			delete[] text;
	}

	wchar_t* Text()
	{
		return text;
	}

	bool Check(int x, int y)
	{
		return (left <= x && x <= right && top <= y && y <= bottom);
	}

	// ���ƽ���
	void Show()
	{
		// ���ݻ���ֵ
		int oldlinecolor = getlinecolor();
		int oldbkcolor = getbkcolor();
		int oldfillcolor = getfillcolor();

		setlinecolor(LIGHTGRAY);		// ���û�����ɫ
		setbkcolor(0xeeeeee);			// ���ñ�����ɫ
		setfillcolor(0xeeeeee);			// ���������ɫ
		fillrectangle(left, top, right, bottom);
		outtextxy(left + 10, top + 5, text);

		// �ָ�����ֵ
		setlinecolor(oldlinecolor);
		setbkcolor(oldbkcolor);
		setfillcolor(oldfillcolor);
	}

	void OnMessage()
	{
		// ���ݻ���ֵ
		int oldlinecolor = getlinecolor();
		int oldbkcolor = getbkcolor();
		int oldfillcolor = getfillcolor();

		setlinecolor(BLACK);			// ���û�����ɫ
		setbkcolor(WHITE);				// ���ñ�����ɫ
		setfillcolor(WHITE);			// ���������ɫ
		fillrectangle(left, top, right, bottom);
		outtextxy(left + 10, top + 5, text);

		int width = textwidth(text);	// �ַ����ܿ��
		int counter = 0;				// �����˸������
		bool binput = true;				// �Ƿ�������

		ExMessage msg;
		while (binput)
		{
			while (binput && peekmessage(&msg, EX_MOUSE | EX_CHAR, false))	// ��ȡ��Ϣ����������Ϣ�����ó�
			{
				if (msg.message == WM_LBUTTONDOWN)
				{
					// ���������ı������棬�����ı�����
					if (msg.x < left || msg.x > right || msg.y < top || msg.y > bottom)
					{
						binput = false;
						break;
					}
				}
				else if (msg.message == WM_CHAR)
				{
					size_t len = wcslen(text);
					switch (msg.ch)
					{
					case '\b':				// �û����˸����ɾ��һ���ַ�
						if (len > 0)
						{
							text[len - 1] = 0;
							width = textwidth(text);
							counter = 0;
							clearrectangle(left + 10 + width, top + 1, right - 1, bottom - 1);
						}
						break;
					case '\r':				// �û����س����������ı�����
					case '\n':
						binput = false;
						break;
					default:				// �û����������������ı�����
						if (len < maxlen - 1)
						{
							text[len++] = msg.ch;
							text[len] = 0;

							clearrectangle(left + 10 + width + 1, top + 3, left + 10 + width + 1, bottom - 3);	// ������Ĺ��
							width = textwidth(text);				// ���¼����ı�����
							counter = 0;
							outtextxy(left + 10, top + 5, text);		// ����µ��ַ���
						}
					}
				}
				peekmessage(NULL, EX_MOUSE | EX_CHAR);				// ����Ϣ���������ոմ������һ����Ϣ
			}

			// ���ƹ�꣨�����˸����Ϊ 20ms * 32��
			counter = (counter + 1) % 32;
			if (counter < 16)
				line(left + 10 + width + 1, top + 3, left + 10 + width + 1, bottom - 3);				// �����
			else
				clearrectangle(left + 10 + width + 1, top + 3, left + 10 + width + 1, bottom - 3);		// �����

			// ��ʱ 20ms
			Sleep(20);
		}

		clearrectangle(left + 10 + width + 1, top + 3, left + 10 + width + 1, bottom - 3);	// �����

		// �ָ�����ֵ
		setlinecolor(oldlinecolor);
		setbkcolor(oldbkcolor);
		setfillcolor(oldfillcolor);

		Show();
	}
};

// ʵ�ְ�ť�ؼ�
class EasyButton
{
private:
	int left = 0, top = 0, right = 0, bottom = 0;	// �ؼ�����
	wchar_t* text = NULL;							// �ؼ�����
	void (*userfunc)() = NULL;						// �ؼ���Ϣ

public:
	void Create(int x1, int y1, int x2, int y2, const wchar_t* title, void (*func)())
	{
		text = new wchar_t[wcslen(title) + 1];
		wcscpy_s(text, wcslen(title) + 1, title);
		left = x1, top = y1, right = x2, bottom = y2;
		userfunc = func;

		// �����û�����
		Show();
	}

	~EasyButton()
	{
		if (text != NULL)
			delete[] text;
	}

	bool Check(int x, int y)
	{
		return (left <= x && x <= right && top <= y && y <= bottom);
	}

	// ���ƽ���
	void Show()
	{
		int oldlinecolor = getlinecolor();
		int oldbkcolor = getbkcolor();
		int oldfillcolor = getfillcolor();

		setlinecolor(BLACK);			// ���û�����ɫ
		setbkcolor(WHITE);				// ���ñ�����ɫ
		setfillcolor(WHITE);			// ���������ɫ
		fillrectangle(left, top, right, bottom);
		outtextxy(left + (right - left - textwidth(text) + 1) / 2, top + (bottom - top - textheight(text) + 1) / 2, text);

		setlinecolor(oldlinecolor);
		setbkcolor(oldbkcolor);
		setfillcolor(oldfillcolor);
	}

	void OnMessage()
	{
		if (userfunc != NULL)
			userfunc();
	}
};

// ����ؼ�
EasyTextBox txtName;
EasyTextBox txtPwd;
EasyTextBox txtEmail;
EasyTextBox txtTitle;
EasyTextBox txtRec;
EasyButton btnOK;
EasyButton btnSend;
EasyButton btnDelete;


void displayEmails();

void On_btnDelete_Click1()
{
	POP3_SSL pop3_ssl_client;
	string server_url = "pop.sina.com";
	string username = "testcn1452@sina.com"; // ���������û���
	string password = "6ed1fa5237676d25";    // ����������Ȩ�룬�Ҵ˴�����ʹ������

	// ���ӵ�������
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// ��¼��������
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	pop3_ssl_client.delete_email(1);
	pop3_ssl_client.close_connection();
	displayEmails();

}

void On_btnDelete_Click2()
{
	POP3_SSL pop3_ssl_client;
	string server_url = "pop.sina.com";
	string username = "testcn1452@sina.com"; // ���������û���
	string password = "6ed1fa5237676d25";    // ����������Ȩ�룬�Ҵ˴�����ʹ������

	// ���ӵ�������
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// ��¼��������
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	pop3_ssl_client.delete_email(2);
	pop3_ssl_client.close_connection();
	displayEmails();
}

void On_btnDelete_Click3()
{
	POP3_SSL pop3_ssl_client;
	string server_url = "pop.sina.com";
	string username = "testcn1452@sina.com"; // ���������û���
	string password = "6ed1fa5237676d25";    // ����������Ȩ�룬�Ҵ˴�����ʹ������

	// ���ӵ�������
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// ��¼��������
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	pop3_ssl_client.delete_email(3);
	pop3_ssl_client.close_connection();
	displayEmails();

}

void On_btnDelete_Click4()
{
	POP3_SSL pop3_ssl_client;
	string server_url = "pop.sina.com";
	string username = "testcn1452@sina.com"; // ���������û���
	string password = "6ed1fa5237676d25";    // ����������Ȩ�룬�Ҵ˴�����ʹ������

	// ���ӵ�������
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// ��¼��������
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	pop3_ssl_client.delete_email(4);
	pop3_ssl_client.close_connection();
	displayEmails();
}

void displayEmails()
{	
	closegraph();
	POP3_SSL pop3_ssl_client;
	string server_url = "pop.sina.com";
	string username = "testcn1452@sina.com"; // ���������û���
	string password = "6ed1fa5237676d25";    // ����������Ȩ�룬�Ҵ˴�����ʹ������

	// ���ӵ�������
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// ��¼��������
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	// ��ȡ�ʼ��б����ʼ��б���ÿһ���ʼ�����������������info.txt�ļ���
	pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);

	// ��txt�ļ�
	std::ifstream file("info.txt");

	// ����ļ��Ƿ�ɹ���
	if (!file.is_open())
	{
		std::cout << "�޷����ļ���" << std::endl;
		return;
	}

	// ��ʼ��easyxͼ�δ���
	initgraph(640, 480);
	IMAGE b;
	loadimage(&b, _T("111.bmp"), 0, 0, false);
	putimage(0, 0, &b);


	// ����������ɫ�ʹ�С
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);
	settextstyle(22, 0, _T("Consolas"));

	// ��ȡtxt�ļ��е���Ϣ�������ư�ť��ͼ�λ�������
	std::string line;
	int y = 50; // ��ť����ʼ������λ��
	int id = 1; // �ʼ����
	
	
	while (std::getline(file, line))
	{
		Email email;
		email.id = id++;
		email.subject = line.substr(35,60);
		email.date = line.substr(91,125);

		// ���ư�ť
		rectangle(50, y - 20, 590, y + 20);

		// �����ʼ����
		string sid = std::to_string(email.id);
		LPCTSTR ssid = str2LPCWSTR(sid);
		outtextxy(60, y, ssid);
		

		string ssub = email.subject;
		LPCTSTR sub = str2LPCWSTR(ssub);

		// �����ʼ�����
		outtextxy(140, y, sub);

		// �����ʼ�����

		string sdate = email.date;
		LPCTSTR ssdate = str2LPCWSTR(sdate);
		outtextxy(260, y, ssdate);

		// ����������λ��
		y += 50;
	}
	
	int x = 50;
	int m = id;
	int n = 1;
	ExMessage msg;
	while (true) {
		if (peekmessage(&msg, EM_MOUSE)) {

			switch (msg.message)
			{
			case WM_LBUTTONDOWN:
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 30 && msg.y <= 70)
				{
					int message_number = 1; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
					pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
					closegraph();
					std::ifstream file("Email1.txt");
					std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
					file.close();
					initgraph(640, 480);
					setbkcolor(WHITE);
					IMAGE b;
					loadimage(&b, _T("111.bmp"), 0, 0, false);
					putimage(0, 0, &b);
					setlinecolor(BLACK);			// ���û�����ɫ
					setbkcolor(WHITE);				// ���ñ�����ɫ
					setfillcolor(WHITE);			// ���������ɫ
					fillrectangle(90, 70, 540, 130);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100,80,con);
					
					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click1);

					
				}
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 80 && msg.y <= 120)
				{
					int message_number = 2; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
					pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
					closegraph();
					std::ifstream file("Email2.txt");
					std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
					file.close();
					initgraph(640, 480);
					setbkcolor(WHITE);
					IMAGE b;
					loadimage(&b, _T("111.bmp"), 0, 0, false);
					putimage(0, 0, &b);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100, 80, con);
					
					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click2);
					
				}
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 130 && msg.y <= 170)
				{
					int message_number = 3; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
					pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
					closegraph();
					std::ifstream file("Email3.txt");
					std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
					file.close();
					initgraph(640, 480);
					setbkcolor(WHITE);
					IMAGE b;
					loadimage(&b, _T("111.bmp"), 0, 0, false);
					putimage(0, 0, &b);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100, 80, con);
					
					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click3);
					
				}
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 180 && msg.y <= 220)
				{
					int message_number = 4; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
					pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
					closegraph();
					std::ifstream file("Email4.txt");
					std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
					file.close();
					initgraph(640, 480);
					setbkcolor(WHITE);
					IMAGE b;
					loadimage(&b, _T("111.bmp"), 0, 0, false);
					putimage(0, 0, &b);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100, 80, con);
					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click4);
					
				}
				break;
			default:
				break;
			}
		}

	}
	// �ر�txt�ļ�
	file.close();

	while (1);
}

void On_btnSend_Click()
{	
	SMTP smtp_client;
	string sender_addr = wide_Char_To_Multi_Byte(txtName.Text());
	string password = "6ed1fa5237676d25";
	vector<string> receiver_addr;
	receiver_addr.push_back(wide_Char_To_Multi_Byte(txtRec.Text()));
	string subject = wide_Char_To_Multi_Byte(txtTitle.Text());
	string data = wide_Char_To_Multi_Byte(txtEmail.Text());
	string msg0 = smtp_client.send_email(sender_addr, password, receiver_addr, subject, data);
	MessageBox(GetHWnd(), L"���ͳɹ�", L"�ɹ�", MB_OK);

	PostgreSQLDatabase db(dbname, user, password0, host, port);
	db.connect();
	db.saveSentEmail(sender_addr, receiver_addr[receiver_addr.size()-1], subject, data);
	if (!db.contactExists(receiver_addr[receiver_addr.size()-1])) {
		std::string contactName; // ������Ը���ʵ�����������ϵ������
		db.saveContact(contactName, receiver_addr[receiver_addr.size()-1]);
	}
}

void displayEmailsSend()
{
	closegraph();

	POP3_SSL pop3_ssl_client;
	string server_url = "pop.sina.com";
	string username = "testcn1452@sina.com"; // ���������û���
	string password = "6ed1fa5237676d25";    // ����������Ȩ�룬�Ҵ˴�����ʹ������

	// ���ӵ�������
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// ��¼��������
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	// ��ȡ�ʼ��б����ʼ��б���ÿһ���ʼ�����������������info.txt�ļ���
	pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);
	
	PostgreSQLDatabase db(dbname, user, password0, host, port);
	db.connect();
	vector<Emails> emails = db.getAllSentEmails();

	int size = emails.size();

	// ��ʼ��easyxͼ�δ���
	initgraph(640, 480);
	IMAGE b;
	loadimage(&b, _T("111.bmp"), 0, 0, false);
	putimage(0, 0, &b);


	// ����������ɫ�ʹ�С
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);
	settextstyle(22, 0, _T("Consolas"));
	int y = 50; // ��ť����ʼ������λ��
	int id = 0;

	while (id<size)
	{
		

		// ���ư�ť
		rectangle(50, y - 20, 590, y + 20);

		// �����ʼ����
		string sid = std::to_string(emails[id].id);
		LPCTSTR ssid = str2LPCWSTR(sid);
		outtextxy(60, y, ssid);


		string ssub = emails[id].subject;
		LPCTSTR sub = str2LPCWSTR(ssub);

		// �����ʼ�����
		outtextxy(140, y, sub);

		// �����ʼ�����

		string sdate = emails[id].timestamp;
		LPCTSTR ssdate = str2LPCWSTR(sdate);
		outtextxy(260, y, ssdate);

		// ����������λ��
		y += 50;
		id++;
	}

	int x = 50;
	int m = id;
	int n = 1;
	ExMessage msg;
	while (true) {
		if (peekmessage(&msg, EM_MOUSE)) {

			switch (msg.message)
			{
			case WM_LBUTTONDOWN:
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 30 && msg.y <= 70)
				{
					int message_number = 1; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
					pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
					closegraph();
					std::ifstream file("Email1.txt");
					std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
					file.close();
					initgraph(640, 480);
					setbkcolor(WHITE);
					IMAGE b;
					loadimage(&b, _T("111.bmp"), 0, 0, false);
					putimage(0, 0, &b);
					setlinecolor(BLACK);			// ���û�����ɫ
					setbkcolor(WHITE);				// ���ñ�����ɫ
					setfillcolor(WHITE);			// ���������ɫ
					fillrectangle(90, 70, 540, 130);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100, 80, con);

					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click1);


				}
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 80 && msg.y <= 120)
				{
					int message_number = 2; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
					pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
					closegraph();
					std::ifstream file("Email2.txt");
					std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
					file.close();
					initgraph(640, 480);
					setbkcolor(WHITE);
					IMAGE b;
					loadimage(&b, _T("111.bmp"), 0, 0, false);
					putimage(0, 0, &b);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100, 80, con);

					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click2);

				}
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 130 && msg.y <= 170)
				{
					int message_number = 3; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
					pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
					closegraph();
					std::ifstream file("Email3.txt");
					std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
					file.close();
					initgraph(640, 480);
					setbkcolor(WHITE);
					IMAGE b;
					loadimage(&b, _T("111.bmp"), 0, 0, false);
					putimage(0, 0, &b);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100, 80, con);

					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click3);

				}
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 180 && msg.y <= 220)
				{
					int message_number = 4; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
					pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
					closegraph();
					std::ifstream file("Email4.txt");
					std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
					file.close();
					initgraph(640, 480);
					setbkcolor(WHITE);
					IMAGE b;
					loadimage(&b, _T("111.bmp"), 0, 0, false);
					putimage(0, 0, &b);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100, 80, con);
					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click4);

				}
				break;
			default:
				break;
			}
		}

	}

	while (1);
}

void display()
{
	closegraph();
	POP3_SSL pop3_ssl_client;
	string server_url = "pop.sina.com";
	string username = "testcn1452@sina.com"; // ���������û���
	string password = "6ed1fa5237676d25";    // ����������Ȩ�룬�Ҵ˴�����ʹ������

	// ���ӵ�������
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// ��¼��������
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	// ��ȡ�ʼ��б����ʼ��б���ÿһ���ʼ�����������������info.txt�ļ���
	pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);

	PostgreSQLDatabase db(dbname, user, password0, host, port);
	db.connect();
	vector<Contact> cont = db.getAllContacts();

	// ��ʼ��easyxͼ�δ���
	initgraph(640, 480);
	IMAGE b;
	loadimage(&b, _T("111.bmp"), 0, 0, false);
	putimage(0, 0, &b);

	// ����������ɫ�ʹ�С
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);
	settextstyle(22, 0, _T("Consolas"));

	// ��ȡtxt�ļ��е���Ϣ�������ư�ť��ͼ�λ�������
	std::string line;
	int y = 50; // ��ť����ʼ������λ��
	int id = 0; // �ʼ����

	int size = cont.size();
	
	while (id<size)
	{
		

		// ���ư�ť
		rectangle(50, y - 20, 590, y + 20);

		// �����ʼ����
		string sid = std::to_string(cont[id].id);
		LPCTSTR ssid = str2LPCWSTR(sid);
		outtextxy(60, y, ssid);


		string ssub = cont[id].name;
		LPCTSTR sub = str2LPCWSTR(ssub);

		// �����ʼ�����
		outtextxy(140, y, sub);

		// �����ʼ�����

		string sdate = cont[id].email;
		LPCTSTR ssdate = str2LPCWSTR(sdate);
		outtextxy(260, y, ssdate);

		// ����������λ��
		y += 50;
		id++;
	}

	while (1);
}

// ��ť btnOK �ĵ���¼�
void On_btnOk_Click()
{	
	//MessageBox(GetHWnd(), L"ok", L"ok", MB_OK);
	if (wcscmp(L"Qwer1234", txtPwd.Text()))
		MessageBox(GetHWnd(), L"�������", L"����", MB_OK);
	else
	{
		wchar_t s[100] = L"Hello, ";
		wcscat_s(s, 100, txtName.Text());
		MessageBox(GetHWnd(), s, L"Hello", MB_OK);
		initgraph(640, 480);
		setbkcolor(BLUE);
		IMAGE b;
		loadimage(&b, _T("111.bmp"), 0, 0, false);
		putimage(0, 0, &b);

		//TCHAR s2[50] = L"�ʼ�����";
		//button0(40, 50, 80, 30,s2 );
		////outtextxy(50, 105, L"�ʼ�����");
		//settextcolor(BLACK);
		//txtEmail.Create(120, 50, 400, 75, 10);

		TCHAR s0[50] = L"��д�ҵ��ʼ�";
		TCHAR s2[50] = L"�鿴�յ��ʼ�";
		TCHAR s3[50] = L"�鿴�����ʼ�";
		TCHAR s4[50] = L"�鿴��ϵ����";

		button(220, 150, 170, 50, s0);
		button(220, 220, 170, 50, s2);
		button(220, 290, 170, 50, s3);
		button(220, 360, 170, 50, s4);

		ExMessage msg;
		while (true) {
			if (peekmessage(&msg, EM_MOUSE)) {

				switch (msg.message)
				{
				case WM_LBUTTONDOWN:
					if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 150 && msg.y <= 150 + 50)
					{
						initgraph(640, 480);
						setbkcolor(0xeeeeee);
						cleardevice();
						settextcolor(BLACK);
						outtextxy(50, 55, L"�����ַ��");
						txtRec.Create(120, 50, 400, 75, 100);			// �����û����ı���ؼ�
						outtextxy(50, 105, L"�ʼ����⣺");
						txtTitle.Create(120, 100, 400, 125, 100);
						outtextxy(50, 155, L"�ʼ����ݣ�");
						txtEmail.Create(120, 150, 400, 175, 100);						// ���������ı���ؼ�
						btnSend.Create(320, 220, 400, 245, L"�����ʼ�", On_btnSend_Click);	// ������ť�ؼ�

						ExMessage msg;
						while (true)
						{
							msg = getmessage(EX_MOUSE);			// ��ȡ��Ϣ����

							if (msg.message == WM_LBUTTONDOWN)
							{
								// �жϿؼ�
								if (txtRec.Check(msg.x, msg.y))				txtRec.OnMessage();

								// �жϿؼ�
								if (txtTitle.Check(msg.x, msg.y))			txtTitle.OnMessage();

								//�жϿؼ�
								if (txtEmail.Check(msg.x, msg.y))			txtEmail.OnMessage();

								// �жϿؼ�
								if (btnSend.Check(msg.x, msg.y)) {
									btnSend.OnMessage();
									initgraph(640, 480);
									setbkcolor(BLUE);
									IMAGE b;
									loadimage(&b, _T("111.bmp"), 0, 0, false);
									putimage(0, 0, &b);

									//TCHAR s2[50] = L"�ʼ�����";
									//button0(40, 50, 80, 30,s2 );
									////outtextxy(50, 105, L"�ʼ�����");
									//settextcolor(BLACK);
									//txtEmail.Create(120, 50, 400, 75, 10);

									TCHAR s0[50] = L"��д�ʼ�";
									TCHAR s2[50] = L"�鿴�ʼ�";

									button(220, 150, 170, 50, s0);
									button(220, 220, 170, 50, s2);

									ExMessage msg;
									while (true) {
										if (peekmessage(&msg, EM_MOUSE)) {

											switch (msg.message)
											{
											case WM_LBUTTONDOWN:
												if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 150 && msg.y <= 150 + 50)
												{
													initgraph(640, 480);
													setbkcolor(0xeeeeee);
													cleardevice();
													settextcolor(BLACK);
													outtextxy(50, 55, L"�����ַ��");
													txtRec.Create(120, 50, 400, 75, 100);			// �����û����ı���ؼ�
													outtextxy(50, 105, L"�ʼ����⣺");
													txtTitle.Create(120, 100, 400, 125, 100);
													outtextxy(50, 155, L"�ʼ����ݣ�");
													txtEmail.Create(120, 150, 400, 175, 100);						// ���������ı���ؼ�
													//btnSend.Create(320, 220, 400, 245, L"�����ʼ�", On_btnSend_Click);	// ������ť�ؼ�
													btnSend.Create(320, 220, 400, 245, L"�����ʼ�",On_btnSend_Click);
													ExMessage msg;
													while (true)
													{
														msg = getmessage(EX_MOUSE);			// ��ȡ��Ϣ����

														if (msg.message == WM_LBUTTONDOWN)
														{
															// �жϿؼ�
															if (txtRec.Check(msg.x, msg.y))				txtRec.OnMessage();

															// �жϿؼ�
															if (txtTitle.Check(msg.x, msg.y))			txtTitle.OnMessage();

															//�жϿؼ�
															if (txtEmail.Check(msg.x, msg.y))			txtEmail.OnMessage();

															// �жϿؼ�
															if (btnSend.Check(msg.x, msg.y)) 			btnSend.OnMessage();
														}
													}

													outtextxy(200, 200, s0);	//д��һ��չʾ���ֵ�Ч�����൱���ǲ���,ʵ��ʹ��ʱ����ɾ��
													//�ڴ˴�д�°�ť���ʱҪִ�еĺ�����ʵ����Ӧ�Ĺ���
												}
												if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 220 && msg.y <= 220 + 50)
												{	
													displayEmails();
													initgraph(640, 480);
													setbkcolor(BLUE);
													IMAGE b;
													loadimage(&b, _T("111.bmp"), 0, 0, false);
													putimage(0, 0, &b);
													outtextxy(200, 200, s2);	//д��һ��չʾ���ֵ�Ч�����൱���ǲ���,ʵ��ʹ��ʱ����ɾ��
													//�ڴ˴�д�°�ť���ʱҪִ�еĺ�����ʵ����Ӧ�Ĺ���
													closegraph();
												}
												break;
											default:
												break;
											}
										}

									}
								}
							}
						}

						outtextxy(200, 200, s0);	//д��һ��չʾ���ֵ�Ч�����൱���ǲ���,ʵ��ʹ��ʱ����ɾ��
						//�ڴ˴�д�°�ť���ʱҪִ�еĺ�����ʵ����Ӧ�Ĺ���
					}
					if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 220 && msg.y <= 220 + 50)
					{
						

						displayEmails();//���յ��ʼ���ʾ

						
						initgraph(640, 480);
						setbkcolor(BLUE);
						IMAGE b;
						loadimage(&b, _T("111.bmp"), 0, 0, false);
						putimage(0, 0, &b);

						outtextxy(200, 200, s2);	//д��һ��չʾ���ֵ�Ч�����൱���ǲ���,ʵ��ʹ��ʱ����ɾ��
						//�ڴ˴�д�°�ť���ʱҪִ�еĺ�����ʵ����Ӧ�Ĺ���
						closegraph();
					}
					if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 290 && msg.y <= 290 + 50)
					{


						displayEmailsSend();//�����ʼ���ʾ


						initgraph(640, 480);
						setbkcolor(BLUE);
						IMAGE b;
						loadimage(&b, _T("111.bmp"), 0, 0, false);
						putimage(0, 0, &b);

						outtextxy(200, 200, s2);	//д��һ��չʾ���ֵ�Ч�����൱���ǲ���,ʵ��ʹ��ʱ����ɾ��
						//�ڴ˴�д�°�ť���ʱҪִ�еĺ�����ʵ����Ӧ�Ĺ���
						closegraph();
					}
					if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 360 && msg.y <= 360 + 50)
					{


						display();


						initgraph(640, 480);
						setbkcolor(BLUE);
						IMAGE b;
						loadimage(&b, _T("111.bmp"), 0, 0, false);
						putimage(0, 0, &b);

						outtextxy(200, 200, s2);	//д��һ��չʾ���ֵ�Ч�����൱���ǲ���,ʵ��ʹ��ʱ����ɾ��
						//�ڴ˴�д�°�ť���ʱҪִ�еĺ�����ʵ����Ӧ�Ĺ���
						closegraph();
					}
					break;
				default:
					break;
				}
			}

		}

	}
}


int main() {
	//SMTP smtp_client;
	//string sender_addr = "testcn1452@sina.com";
	//string password = "6ed1fa5237676d25";
	//vector<string> receiver_addr;
	//receiver_addr.push_back("2803794751@qq.com");
	//string subject = "test";  // �ʼ�����
	//string data = "hello, email"; // �ʼ���������

	//string msg0 = smtp_client.send_email(sender_addr, password, receiver_addr, subject, data);
	//cout << msg0;
	initgraph(640, 480);
    setbkcolor(BLUE);
    IMAGE a;
    loadimage(&a, _T("111.bmp"), 0, 0, false);
    putimage(0, 0, &a);

    

    TCHAR s[50] = L"��¼";
    TCHAR s1[50] = L"�˳�";

    button(220, 150, 170, 50, s);
    button(220, 220, 170, 50, s1);
    ExMessage msg;
    while (true) {
        if (peekmessage(&msg, EM_MOUSE)) {

            switch (msg.message)
            {
            case WM_LBUTTONDOWN:
                if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 150 && msg.y <= 150 + 50)
                {	
					closegraph();
					initgraph(640, 480);
					setbkcolor(0xeeeeee);
					cleardevice();
					settextcolor(BLACK);
					outtextxy(50, 55, L"�����ַ��");
					txtName.Create(120, 50, 400, 75, 100);						// �����û����ı���ؼ�
					outtextxy(50, 105, L"�ܡ� �룺");
					txtPwd.Create(120, 100, 400, 125, 100);						// ���������ı���ؼ�
					btnOK.Create(320, 150, 400, 175, L"OK", On_btnOk_Click);	// ������ť�ؼ�

					ExMessage msg;
					while (true)
					{
						msg = getmessage(EX_MOUSE);			// ��ȡ��Ϣ����

						if (msg.message == WM_LBUTTONDOWN)
						{
							// �жϿؼ�
							if (txtName.Check(msg.x, msg.y))	txtName.OnMessage();

							// �жϿؼ�
							if (txtPwd.Check(msg.x, msg.y))		txtPwd.OnMessage();

							// �жϿؼ�
							if (btnOK.Check(msg.x, msg.y))		btnOK.OnMessage();
						}
					}

                    outtextxy(200, 200, s);	//д��һ��չʾ���ֵ�Ч�����൱���ǲ���,ʵ��ʹ��ʱ����ɾ��
                    //�ڴ˴�д�°�ť���ʱҪִ�еĺ�����ʵ����Ӧ�Ĺ���
                }
                if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 220 && msg.y <= 220 + 50)
                {

                    outtextxy(200, 200, s1);	//д��һ��չʾ���ֵ�Ч�����൱���ǲ���,ʵ��ʹ��ʱ����ɾ��
                    //�ڴ˴�д�°�ť���ʱҪִ�еĺ�����ʵ����Ӧ�Ĺ���
                    closegraph();
                }
                break;
            default:
                break;
            }
        }

    }
	/*SMTP smtp_client;
	string sender_addr = wide_Char_To_Multi_Byte(txtName.Text());
	string password = wide_Char_To_Multi_Byte(txtPwd.Text());
	vector<string> receiver_addr;
	receiver_addr.push_back(wide_Char_To_Multi_Byte(txtRec.Text()));
	string subject = wide_Char_To_Multi_Byte(txtTitle.Text());
	string data = wide_Char_To_Multi_Byte(txtEmail.Text());
	string msg0 = smtp_client.send_email(sender_addr, password, receiver_addr, subject, data);*/
   

	//POP3_SSL pop3_ssl_client;
	//string server_url = "pop.sina.com";
	//string username = "testcn1452@sina.com"; // ���������û���
	//string password = "6ed1fa5237676d25";    // ����������Ȩ�룬�Ҵ˴�����ʹ������

	//// ���ӵ�������
	//if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
	//	cout << "Connection to POP3_SSL server failed." << endl;
	//	return 1;
	//}

	//// ��¼��������
	//if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
	//	cout << "Login to POP3_SSL server failed." << endl;
	//	return 1;
	//}

	//// ��ȡ�ʼ��б����ʼ��б���ÿһ���ʼ�����������������info.txt�ļ���
	//pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);

	//// ����n���ʼ������ݱ��浽email_content.txt��
	//int message_number = 3; // Ҫ�鿴���ʼ��ı�ţ��ĳ��û�����
	//pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);

	////// ɾ����n���ʼ�
	////int delete_message_number = 1;// Ҫɾ�����ʼ��ı�ţ��ĳ��û�����
	////pop3_ssl_client.delete_email(delete_message_number);

	//// �ر����ӣ�ʹɾ��������Ч
	//pop3_ssl_client.close_connection();


    while (1);

    return 0;
}