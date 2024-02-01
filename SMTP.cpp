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
std::string host = "101.37.32.213"; //数据库服务器放在本地
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


// 获取SMTP服务器的URL以及用户名
// URL为发送方的emal地址中‘@’后的字符，并在前添加smtp.
// 用户名为发送方的emal地址中‘@’前的字符
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


// 尝试连接SMTP服务器
bool SMTP::connect_smtp_server(string server_url) {
    int if_error;
    
    // 初始化Winsock库，用于后续套接字操作
	WSADATA wsaData;
    if_error = WSAStartup(MAKEWORD(2, 2), &wsaData);  // 若成功，返回0
	if (if_error != 0){
        WSACleanup();
        return false;
	}

    // 创建套接字
    // AF_INET ：IPv4地址族
    // SOCK_STREAM ：TCP
    // 0 ：默认的协议
    SMTP::smtp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (SMTP::smtp_socket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    HOSTENT *host_entry = gethostbyname(server_url.c_str());
    struct sockaddr_in server_addr; // 服务器地址和端口
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
        return false; // 密码或用户名错误
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

// 初始化Windows Socket API
bool POP3_SSL::initWSA() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	return (result == 0);
}

// 获取服务器信息
bool POP3_SSL::getServerInfo(const string& server_url, addrinfo** server_info) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int result = getaddrinfo(server_url.c_str(), POP3_SSL_PORT, &hints, server_info);
	return (result == 0);
}

// 创建套接字
bool POP3_SSL::createSocket(SOCKET& pop3_socket, addrinfo* server_info) {
	pop3_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	return (pop3_socket != INVALID_SOCKET);
}

// 连接服务器
bool POP3_SSL::connectServer(SOCKET pop3_socket, addrinfo* server_info) {
	int result = connect(pop3_socket, server_info->ai_addr, server_info->ai_addrlen);
	return (result != SOCKET_ERROR);
}

// 初始化SSL
bool POP3_SSL::initSSL(SSL_CTX*& ssl_ctx) {
	ssl_ctx = SSL_CTX_new(TLS_client_method());
	return (ssl_ctx != nullptr);
}

// 建立SSL连接
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

// 连接到POP3服务器
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

	// 初始化SSL
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
	cout << buffer << endl; // 输出服务器的欢迎消息

	// 释放SSL资源
	SSL_CTX_free(ssl_ctx);
	ssl_ctx = NULL;

	// 释放getaddrinfo函数调用时为了获取服务器信息所分配的内存空间
	freeaddrinfo(server_info);

	// 连接成功
	return true;
}

// 登录到POP3服务器
bool POP3_SSL::login_to_pop3_server(const string& username, const string& password) {
	// 身份验证
	string request = "USER " + username + "\r\n";
	SSL_write(ssl_socket, request.c_str(), request.size()); // 发送用户名

	int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE); // 读取用户名响应
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // 打印用户名响应

	request = "PASS " + password + "\r\n";
	SSL_write(ssl_socket, request.c_str(), request.size()); // 发送密码

	bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE); // 读取密码响应
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // 打印密码响应

	// 连接成功并进行身份验证
	return true;
}

// 检索邮件列表
vector<string> POP3_SSL::retrieve_emails() {
	vector<string> emails;
	string request = "LIST\r\n";
	SSL_write(ssl_socket, request.c_str(), request.size());// 发送LIST命令

	int bytes_received = 0;
	string response;
	do {
		bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';
			response += buffer;
		}
	} while (bytes_received > 0 && response.find("\r\n.\r\n") == string::npos); // 循环读取直到收到完整的响应

	cout << response << endl; // 输出完整的邮件列表响应

	// 解析响应获取邮件信息
	size_t start = response.find("\r\n");
	while (start != string::npos) {
		size_t end = response.find("\r\n", start + 2); // 查找下一个换行符
		if (end != string::npos) {
			string email_info = response.substr(start + 2, end - start - 2);
			emails.push_back(email_info);
		}
		start = end;
	}

	return emails;
}

// 检索特定邮件的内容
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



// 提取邮件的标题和日期
void POP3_SSL::extract_email_content(int message_number, const string& response) {
	// 使用正则表达式匹配Subject和date
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

// 将邮件内容存放到文件中
void print_email_info_to_file(const string& email_info) {
	// 使用ofstream创建输出文件流
	ofstream outputFile("email_content.html");
	if (outputFile.is_open()) {
		// 查找HTML内容的起始位置
		size_t start = email_info.find("<");
		if (start != std::string::npos) {
			// 写入HTML内容
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
	cout << "Number of emails: " << emails.size() - 1 << endl; // 会把最后一行的标点符号.当作一个邮件，故减一


	// 打开输出文件流并将cout重定向到文件
	ofstream outputFile("info.txt", ios::binary);
	streambuf* coutBuf = cout.rdbuf();
	cout.rdbuf(outputFile.rdbuf());

	for (int i = 1; i < emails.size(); i++) {
		string email_info = pop3_ssl_client.retrieve_email_content(i);
		pop3_ssl_client.extract_email_content(i, email_info);
	}

	// 关闭文件流并恢复cout
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

// 关闭POP3服务器连接
bool POP3_SSL::close_connection() {
	string quit_command = "QUIT\r\n";
	SSL_write(ssl_socket, quit_command.c_str(), quit_command.size());

	int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // 输出退出命令的响应

	// 关闭SSL套接字
	SSL_shutdown(ssl_socket);
	// SSL_free(ssl_socket);

	// 关闭套接字
	closesocket(pop3_socket);

	// 清理SSL库环境
	//SSL_CTX_free(ssl_ctx);
	//ssl_ctx = NULL;

	return true;
}

// 删除指定邮件
bool POP3_SSL::delete_email(int message_number) {
	string dele_command = "DELE " + to_string(message_number) + "\r\n";
	SSL_write(ssl_socket, dele_command.c_str(), dele_command.size());

	int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
	buffer[bytes_received] = '\0';
	cout << buffer << endl; // 输出删除邮件的响应

	// 检查响应，如果删除成功返回true，否则返回false
	bool deleted = (strstr(buffer, "+OK") != nullptr);

	return deleted;
}

void button(int x, int y, int w, int h, TCHAR* text) {
    setbkmode(TRANSPARENT);
    setfillcolor(GREEN);
    fillroundrect(x, y, x + w, y + h, 10, 10);

    TCHAR s1[] = L"黑体";
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

	TCHAR s1[] = L"黑体";
	settextstyle(15, 0, s1);
	TCHAR s[50] = L"hello";

	int tx = x + (w - textwidth(text)) / 2;
	int ty = y + (h - textheight(text)) / 2;
	outtextxy(tx, ty, text);

}

string wide_Char_To_Multi_Byte(wchar_t* pWCStrKey)
{
	//第一次调用确认转换后单字节字符串的长度，用于开辟空间
	int pSize = WideCharToMultiByte(CP_OEMCP, 0, pWCStrKey, wcslen(pWCStrKey), NULL, 0, NULL, NULL);
	char* pCStrKey = new char[pSize + 1];
	//第二次调用将双字节字符串转换成单字节字符串
	WideCharToMultiByte(CP_OEMCP, 0, pWCStrKey, wcslen(pWCStrKey), pCStrKey, pSize, NULL, NULL);
	pCStrKey[pSize] = '\0';
	

	//如果想要转换成string，直接赋值即可
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
	int left = 0, top = 0, right = 0, bottom = 0;	// 控件坐标
	wchar_t* text = NULL;							// 控件内容
	size_t maxlen = 0;									// 文本框最大内容长度

public:
	void Create(int x1, int y1, int x2, int y2, int max)
	{
		maxlen = max;
		text = new wchar_t[maxlen];
		text[0] = 0;
		left = x1, top = y1, right = x2, bottom = y2;

		// 绘制用户界面
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

	// 绘制界面
	void Show()
	{
		// 备份环境值
		int oldlinecolor = getlinecolor();
		int oldbkcolor = getbkcolor();
		int oldfillcolor = getfillcolor();

		setlinecolor(LIGHTGRAY);		// 设置画线颜色
		setbkcolor(0xeeeeee);			// 设置背景颜色
		setfillcolor(0xeeeeee);			// 设置填充颜色
		fillrectangle(left, top, right, bottom);
		outtextxy(left + 10, top + 5, text);

		// 恢复环境值
		setlinecolor(oldlinecolor);
		setbkcolor(oldbkcolor);
		setfillcolor(oldfillcolor);
	}

	void OnMessage()
	{
		// 备份环境值
		int oldlinecolor = getlinecolor();
		int oldbkcolor = getbkcolor();
		int oldfillcolor = getfillcolor();

		setlinecolor(BLACK);			// 设置画线颜色
		setbkcolor(WHITE);				// 设置背景颜色
		setfillcolor(WHITE);			// 设置填充颜色
		fillrectangle(left, top, right, bottom);
		outtextxy(left + 10, top + 5, text);

		int width = textwidth(text);	// 字符串总宽度
		int counter = 0;				// 光标闪烁计数器
		bool binput = true;				// 是否输入中

		ExMessage msg;
		while (binput)
		{
			while (binput && peekmessage(&msg, EX_MOUSE | EX_CHAR, false))	// 获取消息，但不从消息队列拿出
			{
				if (msg.message == WM_LBUTTONDOWN)
				{
					// 如果鼠标点击文本框外面，结束文本输入
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
					case '\b':				// 用户按退格键，删掉一个字符
						if (len > 0)
						{
							text[len - 1] = 0;
							width = textwidth(text);
							counter = 0;
							clearrectangle(left + 10 + width, top + 1, right - 1, bottom - 1);
						}
						break;
					case '\r':				// 用户按回车键，结束文本输入
					case '\n':
						binput = false;
						break;
					default:				// 用户按其它键，接受文本输入
						if (len < maxlen - 1)
						{
							text[len++] = msg.ch;
							text[len] = 0;

							clearrectangle(left + 10 + width + 1, top + 3, left + 10 + width + 1, bottom - 3);	// 清除画的光标
							width = textwidth(text);				// 重新计算文本框宽度
							counter = 0;
							outtextxy(left + 10, top + 5, text);		// 输出新的字符串
						}
					}
				}
				peekmessage(NULL, EX_MOUSE | EX_CHAR);				// 从消息队列抛弃刚刚处理过的一个消息
			}

			// 绘制光标（光标闪烁周期为 20ms * 32）
			counter = (counter + 1) % 32;
			if (counter < 16)
				line(left + 10 + width + 1, top + 3, left + 10 + width + 1, bottom - 3);				// 画光标
			else
				clearrectangle(left + 10 + width + 1, top + 3, left + 10 + width + 1, bottom - 3);		// 擦光标

			// 延时 20ms
			Sleep(20);
		}

		clearrectangle(left + 10 + width + 1, top + 3, left + 10 + width + 1, bottom - 3);	// 擦光标

		// 恢复环境值
		setlinecolor(oldlinecolor);
		setbkcolor(oldbkcolor);
		setfillcolor(oldfillcolor);

		Show();
	}
};

// 实现按钮控件
class EasyButton
{
private:
	int left = 0, top = 0, right = 0, bottom = 0;	// 控件坐标
	wchar_t* text = NULL;							// 控件内容
	void (*userfunc)() = NULL;						// 控件消息

public:
	void Create(int x1, int y1, int x2, int y2, const wchar_t* title, void (*func)())
	{
		text = new wchar_t[wcslen(title) + 1];
		wcscpy_s(text, wcslen(title) + 1, title);
		left = x1, top = y1, right = x2, bottom = y2;
		userfunc = func;

		// 绘制用户界面
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

	// 绘制界面
	void Show()
	{
		int oldlinecolor = getlinecolor();
		int oldbkcolor = getbkcolor();
		int oldfillcolor = getfillcolor();

		setlinecolor(BLACK);			// 设置画线颜色
		setbkcolor(WHITE);				// 设置背景颜色
		setfillcolor(WHITE);			// 设置填充颜色
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

// 定义控件
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
	string username = "testcn1452@sina.com"; // 新浪邮箱用户名
	string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码

	// 连接到服务器
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// 登录到服务器
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
	string username = "testcn1452@sina.com"; // 新浪邮箱用户名
	string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码

	// 连接到服务器
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// 登录到服务器
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
	string username = "testcn1452@sina.com"; // 新浪邮箱用户名
	string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码

	// 连接到服务器
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// 登录到服务器
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
	string username = "testcn1452@sina.com"; // 新浪邮箱用户名
	string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码

	// 连接到服务器
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// 登录到服务器
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
	string username = "testcn1452@sina.com"; // 新浪邮箱用户名
	string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码

	// 连接到服务器
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// 登录到服务器
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	// 获取邮件列表，将邮件列表中每一封邮件的主题和日期输出到info.txt文件中
	pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);

	// 打开txt文件
	std::ifstream file("info.txt");

	// 检查文件是否成功打开
	if (!file.is_open())
	{
		std::cout << "无法打开文件！" << std::endl;
		return;
	}

	// 初始化easyx图形窗口
	initgraph(640, 480);
	IMAGE b;
	loadimage(&b, _T("111.bmp"), 0, 0, false);
	putimage(0, 0, &b);


	// 设置字体颜色和大小
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);
	settextstyle(22, 0, _T("Consolas"));

	// 读取txt文件中的信息，并绘制按钮到图形化界面中
	std::string line;
	int y = 50; // 按钮的起始纵坐标位置
	int id = 1; // 邮件编号
	
	
	while (std::getline(file, line))
	{
		Email email;
		email.id = id++;
		email.subject = line.substr(35,60);
		email.date = line.substr(91,125);

		// 绘制按钮
		rectangle(50, y - 20, 590, y + 20);

		// 绘制邮件编号
		string sid = std::to_string(email.id);
		LPCTSTR ssid = str2LPCWSTR(sid);
		outtextxy(60, y, ssid);
		

		string ssub = email.subject;
		LPCTSTR sub = str2LPCWSTR(ssub);

		// 绘制邮件主题
		outtextxy(140, y, sub);

		// 绘制邮件日期

		string sdate = email.date;
		LPCTSTR ssdate = str2LPCWSTR(sdate);
		outtextxy(260, y, ssdate);

		// 更新纵坐标位置
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
					int message_number = 1; // 要查看的邮件的编号，改成用户输入
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
					setlinecolor(BLACK);			// 设置画线颜色
					setbkcolor(WHITE);				// 设置背景颜色
					setfillcolor(WHITE);			// 设置填充颜色
					fillrectangle(90, 70, 540, 130);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100,80,con);
					
					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click1);

					
				}
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 80 && msg.y <= 120)
				{
					int message_number = 2; // 要查看的邮件的编号，改成用户输入
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
					int message_number = 3; // 要查看的邮件的编号，改成用户输入
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
					int message_number = 4; // 要查看的邮件的编号，改成用户输入
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
	// 关闭txt文件
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
	MessageBox(GetHWnd(), L"发送成功", L"成功", MB_OK);

	PostgreSQLDatabase db(dbname, user, password0, host, port);
	db.connect();
	db.saveSentEmail(sender_addr, receiver_addr[receiver_addr.size()-1], subject, data);
	if (!db.contactExists(receiver_addr[receiver_addr.size()-1])) {
		std::string contactName; // 这里可以根据实际情况设置联系人姓名
		db.saveContact(contactName, receiver_addr[receiver_addr.size()-1]);
	}
}

void displayEmailsSend()
{
	closegraph();

	POP3_SSL pop3_ssl_client;
	string server_url = "pop.sina.com";
	string username = "testcn1452@sina.com"; // 新浪邮箱用户名
	string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码

	// 连接到服务器
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// 登录到服务器
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	// 获取邮件列表，将邮件列表中每一封邮件的主题和日期输出到info.txt文件中
	pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);
	
	PostgreSQLDatabase db(dbname, user, password0, host, port);
	db.connect();
	vector<Emails> emails = db.getAllSentEmails();

	int size = emails.size();

	// 初始化easyx图形窗口
	initgraph(640, 480);
	IMAGE b;
	loadimage(&b, _T("111.bmp"), 0, 0, false);
	putimage(0, 0, &b);


	// 设置字体颜色和大小
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);
	settextstyle(22, 0, _T("Consolas"));
	int y = 50; // 按钮的起始纵坐标位置
	int id = 0;

	while (id<size)
	{
		

		// 绘制按钮
		rectangle(50, y - 20, 590, y + 20);

		// 绘制邮件编号
		string sid = std::to_string(emails[id].id);
		LPCTSTR ssid = str2LPCWSTR(sid);
		outtextxy(60, y, ssid);


		string ssub = emails[id].subject;
		LPCTSTR sub = str2LPCWSTR(ssub);

		// 绘制邮件主题
		outtextxy(140, y, sub);

		// 绘制邮件日期

		string sdate = emails[id].timestamp;
		LPCTSTR ssdate = str2LPCWSTR(sdate);
		outtextxy(260, y, ssdate);

		// 更新纵坐标位置
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
					int message_number = 1; // 要查看的邮件的编号，改成用户输入
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
					setlinecolor(BLACK);			// 设置画线颜色
					setbkcolor(WHITE);				// 设置背景颜色
					setfillcolor(WHITE);			// 设置填充颜色
					fillrectangle(90, 70, 540, 130);
					settextcolor(BLACK);
					LPCTSTR con = str2LPCWSTR(content);
					outtextxy(100, 80, con);

					btnDelete.Create(320, 150, 400, 175, L"Delete", On_btnDelete_Click1);


				}
				if (msg.x >= 50 && msg.x <= 590 && msg.y >= 80 && msg.y <= 120)
				{
					int message_number = 2; // 要查看的邮件的编号，改成用户输入
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
					int message_number = 3; // 要查看的邮件的编号，改成用户输入
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
					int message_number = 4; // 要查看的邮件的编号，改成用户输入
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
	string username = "testcn1452@sina.com"; // 新浪邮箱用户名
	string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码

	// 连接到服务器
	if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
		cout << "Connection to POP3_SSL server failed." << endl;
		return;
	}

	// 登录到服务器
	if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
		cout << "Login to POP3_SSL server failed." << endl;
		return;
	}

	// 获取邮件列表，将邮件列表中每一封邮件的主题和日期输出到info.txt文件中
	pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);

	PostgreSQLDatabase db(dbname, user, password0, host, port);
	db.connect();
	vector<Contact> cont = db.getAllContacts();

	// 初始化easyx图形窗口
	initgraph(640, 480);
	IMAGE b;
	loadimage(&b, _T("111.bmp"), 0, 0, false);
	putimage(0, 0, &b);

	// 设置字体颜色和大小
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);
	settextstyle(22, 0, _T("Consolas"));

	// 读取txt文件中的信息，并绘制按钮到图形化界面中
	std::string line;
	int y = 50; // 按钮的起始纵坐标位置
	int id = 0; // 邮件编号

	int size = cont.size();
	
	while (id<size)
	{
		

		// 绘制按钮
		rectangle(50, y - 20, 590, y + 20);

		// 绘制邮件编号
		string sid = std::to_string(cont[id].id);
		LPCTSTR ssid = str2LPCWSTR(sid);
		outtextxy(60, y, ssid);


		string ssub = cont[id].name;
		LPCTSTR sub = str2LPCWSTR(ssub);

		// 绘制邮件主题
		outtextxy(140, y, sub);

		// 绘制邮件日期

		string sdate = cont[id].email;
		LPCTSTR ssdate = str2LPCWSTR(sdate);
		outtextxy(260, y, ssdate);

		// 更新纵坐标位置
		y += 50;
		id++;
	}

	while (1);
}

// 按钮 btnOK 的点击事件
void On_btnOk_Click()
{	
	//MessageBox(GetHWnd(), L"ok", L"ok", MB_OK);
	if (wcscmp(L"Qwer1234", txtPwd.Text()))
		MessageBox(GetHWnd(), L"密码错误", L"错误", MB_OK);
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

		//TCHAR s2[50] = L"邮件内容";
		//button0(40, 50, 80, 30,s2 );
		////outtextxy(50, 105, L"邮件内容");
		//settextcolor(BLACK);
		//txtEmail.Create(120, 50, 400, 75, 10);

		TCHAR s0[50] = L"编写我的邮件";
		TCHAR s2[50] = L"查看收到邮件";
		TCHAR s3[50] = L"查看发送邮件";
		TCHAR s4[50] = L"查看联系邮箱";

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
						outtextxy(50, 55, L"邮箱地址：");
						txtRec.Create(120, 50, 400, 75, 100);			// 创建用户名文本框控件
						outtextxy(50, 105, L"邮件标题：");
						txtTitle.Create(120, 100, 400, 125, 100);
						outtextxy(50, 155, L"邮件内容：");
						txtEmail.Create(120, 150, 400, 175, 100);						// 创建密码文本框控件
						btnSend.Create(320, 220, 400, 245, L"发送邮件", On_btnSend_Click);	// 创建按钮控件

						ExMessage msg;
						while (true)
						{
							msg = getmessage(EX_MOUSE);			// 获取消息输入

							if (msg.message == WM_LBUTTONDOWN)
							{
								// 判断控件
								if (txtRec.Check(msg.x, msg.y))				txtRec.OnMessage();

								// 判断控件
								if (txtTitle.Check(msg.x, msg.y))			txtTitle.OnMessage();

								//判断控件
								if (txtEmail.Check(msg.x, msg.y))			txtEmail.OnMessage();

								// 判断控件
								if (btnSend.Check(msg.x, msg.y)) {
									btnSend.OnMessage();
									initgraph(640, 480);
									setbkcolor(BLUE);
									IMAGE b;
									loadimage(&b, _T("111.bmp"), 0, 0, false);
									putimage(0, 0, &b);

									//TCHAR s2[50] = L"邮件内容";
									//button0(40, 50, 80, 30,s2 );
									////outtextxy(50, 105, L"邮件内容");
									//settextcolor(BLACK);
									//txtEmail.Create(120, 50, 400, 75, 10);

									TCHAR s0[50] = L"编写邮件";
									TCHAR s2[50] = L"查看邮件";

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
													outtextxy(50, 55, L"邮箱地址：");
													txtRec.Create(120, 50, 400, 75, 100);			// 创建用户名文本框控件
													outtextxy(50, 105, L"邮件标题：");
													txtTitle.Create(120, 100, 400, 125, 100);
													outtextxy(50, 155, L"邮件内容：");
													txtEmail.Create(120, 150, 400, 175, 100);						// 创建密码文本框控件
													//btnSend.Create(320, 220, 400, 245, L"发送邮件", On_btnSend_Click);	// 创建按钮控件
													btnSend.Create(320, 220, 400, 245, L"发送邮件",On_btnSend_Click);
													ExMessage msg;
													while (true)
													{
														msg = getmessage(EX_MOUSE);			// 获取消息输入

														if (msg.message == WM_LBUTTONDOWN)
														{
															// 判断控件
															if (txtRec.Check(msg.x, msg.y))				txtRec.OnMessage();

															// 判断控件
															if (txtTitle.Check(msg.x, msg.y))			txtTitle.OnMessage();

															//判断控件
															if (txtEmail.Check(msg.x, msg.y))			txtEmail.OnMessage();

															// 判断控件
															if (btnSend.Check(msg.x, msg.y)) 			btnSend.OnMessage();
														}
													}

													outtextxy(200, 200, s0);	//写了一个展示文字的效果，相当于是测试,实际使用时可以删除
													//在此处写下按钮点击时要执行的函数，实现相应的功能
												}
												if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 220 && msg.y <= 220 + 50)
												{	
													displayEmails();
													initgraph(640, 480);
													setbkcolor(BLUE);
													IMAGE b;
													loadimage(&b, _T("111.bmp"), 0, 0, false);
													putimage(0, 0, &b);
													outtextxy(200, 200, s2);	//写了一个展示文字的效果，相当于是测试,实际使用时可以删除
													//在此处写下按钮点击时要执行的函数，实现相应的功能
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

						outtextxy(200, 200, s0);	//写了一个展示文字的效果，相当于是测试,实际使用时可以删除
						//在此处写下按钮点击时要执行的函数，实现相应的功能
					}
					if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 220 && msg.y <= 220 + 50)
					{
						

						displayEmails();//已收到邮件显示

						
						initgraph(640, 480);
						setbkcolor(BLUE);
						IMAGE b;
						loadimage(&b, _T("111.bmp"), 0, 0, false);
						putimage(0, 0, &b);

						outtextxy(200, 200, s2);	//写了一个展示文字的效果，相当于是测试,实际使用时可以删除
						//在此处写下按钮点击时要执行的函数，实现相应的功能
						closegraph();
					}
					if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 290 && msg.y <= 290 + 50)
					{


						displayEmailsSend();//发送邮件显示


						initgraph(640, 480);
						setbkcolor(BLUE);
						IMAGE b;
						loadimage(&b, _T("111.bmp"), 0, 0, false);
						putimage(0, 0, &b);

						outtextxy(200, 200, s2);	//写了一个展示文字的效果，相当于是测试,实际使用时可以删除
						//在此处写下按钮点击时要执行的函数，实现相应的功能
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

						outtextxy(200, 200, s2);	//写了一个展示文字的效果，相当于是测试,实际使用时可以删除
						//在此处写下按钮点击时要执行的函数，实现相应的功能
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
	//string subject = "test";  // 邮件标题
	//string data = "hello, email"; // 邮件正文内容

	//string msg0 = smtp_client.send_email(sender_addr, password, receiver_addr, subject, data);
	//cout << msg0;
	initgraph(640, 480);
    setbkcolor(BLUE);
    IMAGE a;
    loadimage(&a, _T("111.bmp"), 0, 0, false);
    putimage(0, 0, &a);

    

    TCHAR s[50] = L"登录";
    TCHAR s1[50] = L"退出";

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
					outtextxy(50, 55, L"邮箱地址：");
					txtName.Create(120, 50, 400, 75, 100);						// 创建用户名文本框控件
					outtextxy(50, 105, L"密　 码：");
					txtPwd.Create(120, 100, 400, 125, 100);						// 创建密码文本框控件
					btnOK.Create(320, 150, 400, 175, L"OK", On_btnOk_Click);	// 创建按钮控件

					ExMessage msg;
					while (true)
					{
						msg = getmessage(EX_MOUSE);			// 获取消息输入

						if (msg.message == WM_LBUTTONDOWN)
						{
							// 判断控件
							if (txtName.Check(msg.x, msg.y))	txtName.OnMessage();

							// 判断控件
							if (txtPwd.Check(msg.x, msg.y))		txtPwd.OnMessage();

							// 判断控件
							if (btnOK.Check(msg.x, msg.y))		btnOK.OnMessage();
						}
					}

                    outtextxy(200, 200, s);	//写了一个展示文字的效果，相当于是测试,实际使用时可以删除
                    //在此处写下按钮点击时要执行的函数，实现相应的功能
                }
                if (msg.x >= 220 && msg.x <= 220 + 170 && msg.y >= 220 && msg.y <= 220 + 50)
                {

                    outtextxy(200, 200, s1);	//写了一个展示文字的效果，相当于是测试,实际使用时可以删除
                    //在此处写下按钮点击时要执行的函数，实现相应的功能
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
	//string username = "testcn1452@sina.com"; // 新浪邮箱用户名
	//string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码

	//// 连接到服务器
	//if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
	//	cout << "Connection to POP3_SSL server failed." << endl;
	//	return 1;
	//}

	//// 登录到服务器
	//if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
	//	cout << "Login to POP3_SSL server failed." << endl;
	//	return 1;
	//}

	//// 获取邮件列表，将邮件列表中每一封邮件的主题和日期输出到info.txt文件中
	//pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);

	//// 将第n封邮件的内容保存到email_content.txt中
	//int message_number = 3; // 要查看的邮件的编号，改成用户输入
	//pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);

	////// 删除第n封邮件
	////int delete_message_number = 1;// 要删除的邮件的编号，改成用户输入
	////pop3_ssl_client.delete_email(delete_message_number);

	//// 关闭连接，使删除操作生效
	//pop3_ssl_client.close_connection();


    while (1);

    return 0;
}