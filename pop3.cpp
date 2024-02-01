//#include <regex>
//#include <fstream>
//#include <iomanip>
//#include <locale>
//#include "pop3.h"
//#include "base64.h"
//
//// 初始化Windows Socket API
//bool POP3_SSL::initWSA() {
//    WSADATA wsaData;
//    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
//    return (result == 0);
//}
//
//// 获取服务器信息
//bool POP3_SSL::getServerInfo(const string& server_url, addrinfo** server_info) {
//    struct addrinfo hints;
//    memset(&hints, 0, sizeof(hints));
//    hints.ai_family = AF_INET;
//    hints.ai_socktype = SOCK_STREAM;
//
//    int result = getaddrinfo(server_url.c_str(), POP3_SSL_PORT, &hints, server_info);
//    return (result == 0);
//}
//
//// 创建套接字
//bool POP3_SSL::createSocket(SOCKET& pop3_socket, addrinfo* server_info) {
//    pop3_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
//    return (pop3_socket != INVALID_SOCKET);
//}
//
//// 连接服务器
//bool POP3_SSL::connectServer(SOCKET pop3_socket, addrinfo* server_info) {
//    int result = connect(pop3_socket, server_info->ai_addr, server_info->ai_addrlen);
//    return (result != SOCKET_ERROR);
//}
//
//// 初始化SSL
//bool POP3_SSL::initSSL(SSL_CTX*& ssl_ctx) {
//    ssl_ctx = SSL_CTX_new(TLS_client_method());
//    return (ssl_ctx != nullptr);
//}
//
//// 建立SSL连接
//bool POP3_SSL::setupSSL(SSL** ssl_socket, SOCKET pop3_socket, SSL_CTX* ssl_ctx) {
//    *ssl_socket = SSL_new(ssl_ctx);
//    if (!(*ssl_socket)) {
//        SSL_CTX_free(ssl_ctx);
//        return false;
//    }
//
//    if (SSL_set_fd(*ssl_socket, pop3_socket) != 1) {
//        SSL_CTX_free(ssl_ctx);
//        return false;
//    }
//
//    if (SSL_connect(*ssl_socket) != 1) {
//        SSL_CTX_free(ssl_ctx);
//        return false;
//    }
//
//    return true;
//}
//
//// 连接到POP3服务器
//bool POP3_SSL::connect_to_pop3_server(const string& server_url) {
//    WSADATA wsaData;
//    if (!initWSA()) {
//        return false;
//    }
//
//    addrinfo* server_info;
//    if (!getServerInfo(server_url, &server_info)) {
//        WSACleanup();
//        return false;
//    }
//
//    if (!createSocket(pop3_socket, server_info)) {
//        freeaddrinfo(server_info);
//        WSACleanup();
//        return false;
//    }
//
//    if (!connectServer(pop3_socket, server_info)) {
//        freeaddrinfo(server_info);
//        cleanup();
//        return false;
//    }
//
//    // 初始化SSL
//    SSL_CTX* ssl_ctx;
//    if (!initSSL(ssl_ctx)) {
//        cleanup();
//        return false;
//    }
//
//    if (!setupSSL(&ssl_socket, pop3_socket, ssl_ctx)) {
//        cleanup();
//        return false;
//    }
//
//    int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
//    buffer[bytes_received] = '\0';
//    cout << buffer << endl; // 输出服务器的欢迎消息
//
//    // 释放SSL资源
//    SSL_CTX_free(ssl_ctx);
//    ssl_ctx = NULL;
//
//    // 释放getaddrinfo函数调用时为了获取服务器信息所分配的内存空间
//    freeaddrinfo(server_info);
//
//    // 连接成功
//    return true;
//}
//
//// 登录到POP3服务器
//bool POP3_SSL::login_to_pop3_server(const string& username, const string& password) {
//    // 身份验证
//    string request = "USER " + username + "\r\n";
//    SSL_write(ssl_socket, request.c_str(), request.size()); // 发送用户名
//
//    int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE); // 读取用户名响应
//    buffer[bytes_received] = '\0';
//    cout << buffer << endl; // 打印用户名响应
//
//    request = "PASS " + password + "\r\n";
//    SSL_write(ssl_socket, request.c_str(), request.size()); // 发送密码
//
//    bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE); // 读取密码响应
//    buffer[bytes_received] = '\0';
//    cout << buffer << endl; // 打印密码响应
//
//    // 连接成功并进行身份验证
//    return true;
//}
//
//// 检索邮件列表
//vector<string> POP3_SSL::retrieve_emails() {
//    vector<string> emails;
//    string request = "LIST\r\n";
//    SSL_write(ssl_socket, request.c_str(), request.size());// 发送LIST命令
//
//    int bytes_received = 0;
//    string response;
//    do {
//        bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
//        if (bytes_received > 0) {
//            buffer[bytes_received] = '\0';
//            response += buffer;
//        }
//    } while (bytes_received > 0 && response.find("\r\n.\r\n") == string::npos); // 循环读取直到收到完整的响应
//
//    cout << response << endl; // 输出完整的邮件列表响应
//
//    // 解析响应获取邮件信息
//    size_t start = response.find("\r\n");
//    while (start != string::npos) {
//        size_t end = response.find("\r\n", start + 2); // 查找下一个换行符
//        if (end != string::npos) {
//            string email_info = response.substr(start + 2, end - start - 2);
//            emails.push_back(email_info);
//        }
//        start = end;
//    }
//
//    return emails;
//}
//
//// 检索特定邮件的内容
//string POP3_SSL::retrieve_email_content(int message_number) {
//    string retr_command = "RETR " + to_string(message_number) + "\r\n";
//    SSL_write(ssl_socket, retr_command.c_str(), retr_command.size());
//
//    string email_content;
//    char temp_buffer[BUFFER_SIZE];
//    int bytes_received;
//    do {
//        bytes_received = SSL_read(ssl_socket, temp_buffer, BUFFER_SIZE);
//        if (bytes_received > 0) {
//            temp_buffer[bytes_received] = '\0';
//            email_content += temp_buffer;
//        }
//    } while (bytes_received > 0 && email_content.find("\r\n.\r\n") == string::npos);
//
//    return email_content;
//}
//
//
//
//// 提取邮件的标题和日期
//void POP3_SSL::extract_email_content(int message_number, const string& response) {
//    // 使用正则表达式匹配Subject和date
//    regex subjectRegexCN("Subject: =\\?utf-8\\?B\\?(.+)\\?=\r\n", regex_constants::icase);
//    regex subjectRegexEN("Subject: (.+)\r\n");
//    regex dateRegex("Date: (.+)\r\n");
//
//    cout << "Email:" << left << setw(20) << message_number;
//
//    // research + base64_decode
//    smatch matches;
//    // subject
//    if (regex_search(response, matches, subjectRegexCN)) {
//        string subject = matches[1];
//        string decoded_subject = base64_decode(subject);
//        // wstring str = to_wstring(decoded_subject);
//        cout << "Subject: " << left << hex << setw(50) << decoded_subject;
//    }
//    else if (regex_search(response, matches, subjectRegexEN)) {
//        string subject = matches[1];
//        cout << "Subject: " << left << setw(50) << subject;
//    }
//
//    // date
//    if (regex_search(response, matches, dateRegex)) {
//        string date = matches[1];
//        cout << "Date: " << date << endl;
//    }
//
//}
//
//// 将邮件内容存放到文件中
//void print_email_info_to_file(const string& email_info) {
//    // 使用ofstream创建输出文件流
//    ofstream outputFile("email_content.html");
//    if (outputFile.is_open()) {
//        // 查找HTML内容的起始位置
//        size_t start = email_info.find("<");
//        if (start != std::string::npos) {
//            // 写入HTML内容
//            outputFile << email_info.substr(start) << std::endl;
//        }
//        outputFile.close();
//        cout << "Email content saved to 'email_content.html'." << endl;
//    }
//    else {
//        cout << "Error: Unable to open file for writing." << endl;
//    }
//}
//
//void POP3_SSL::retrieve_and_print_email_info(POP3_SSL & pop3_ssl_client) {
//    vector<string> emails = pop3_ssl_client.retrieve_emails();
//    cout << "Number of emails: " << emails.size() - 1 << endl; // 会把最后一行的标点符号.当作一个邮件，故减一
//
//
//    // 打开输出文件流并将cout重定向到文件
//    ofstream outputFile("info.txt", ios::binary);
//    streambuf* coutBuf = cout.rdbuf();
//    cout.rdbuf(outputFile.rdbuf());
//
//    for (int i = 1; i < emails.size(); i++) {
//        string email_info = pop3_ssl_client.retrieve_email_content(i);
//        pop3_ssl_client.extract_email_content(i, email_info);
//    }
//
//    // 关闭文件流并恢复cout
//    outputFile.close();
//    cout.rdbuf(coutBuf);
//
//}
//
//void POP3_SSL::save_email_content(POP3_SSL& pop3_ssl_client, int message_number) {
//    vector<string> emails = pop3_ssl_client.retrieve_emails();
//    if (emails.size() <= 1 || message_number >= emails.size()) {
//        cout << "Error: Invalid message number." << endl;
//        return;
//    }
//
//    string email_content = pop3_ssl_client.retrieve_email_content(message_number);
//
//    regex contentRegex("Content-Transfer-Encoding: base64\r\n\r\n(.*?)\r\n\r\n",
//        std::regex_constants::icase);
//    smatch matches;
//    regex_search(email_content, matches, contentRegex);
//    string email_txt_encoded = matches[1];
//    string email_txt = base64_decode(email_txt_encoded);
//
//    // Save Email Content to File
//    string Filename = "Email" + to_string(message_number) + ".txt";
//    ofstream emailContentFile(Filename);
//    if (emailContentFile.is_open()) {
//        emailContentFile << email_txt;
//        emailContentFile.close();
//        cout << "Email content saved to " << Filename << endl;
//    }
//    else {
//        cout << "Error: Unable to open file for writing." << endl;
//    }
//}
//
//// 关闭POP3服务器连接
//bool POP3_SSL::close_connection() {
//    string quit_command = "QUIT\r\n";
//    SSL_write(ssl_socket, quit_command.c_str(), quit_command.size());
//
//    int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
//    buffer[bytes_received] = '\0';
//    cout << buffer << endl; // 输出退出命令的响应
//
//    // 关闭SSL套接字
//    SSL_shutdown(ssl_socket);
//    // SSL_free(ssl_socket);
//
//    // 关闭套接字
//    closesocket(pop3_socket);
//
//    // 清理SSL库环境
//    //SSL_CTX_free(ssl_ctx);
//    //ssl_ctx = NULL;
//
//    return true;
//}
//
//// 删除指定邮件
//bool POP3_SSL::delete_email(int message_number) {
//    string dele_command = "DELE " + to_string(message_number) + "\r\n";
//    SSL_write(ssl_socket, dele_command.c_str(), dele_command.size());
//
//    int bytes_received = SSL_read(ssl_socket, buffer, BUFFER_SIZE);
//    buffer[bytes_received] = '\0';
//    cout << buffer << endl; // 输出删除邮件的响应
//
//    // 检查响应，如果删除成功返回true，否则返回false
//    bool deleted = (strstr(buffer, "+OK") != nullptr);
//
//    return deleted;
//}
//
////int main() {
////    POP3_SSL pop3_ssl_client;
////    string server_url = "pop.sina.com";
////    string username = "testcn1452@sina.com"; // 新浪邮箱用户名
////    string password = "6ed1fa5237676d25";    // 新浪邮箱授权码，且此处不能使用密码
////
////    // 连接到服务器
////    if (!pop3_ssl_client.connect_to_pop3_server(server_url)) {
////        cout << "Connection to POP3_SSL server failed." << endl;
////        return 1;
////    }
////
////    // 登录到服务器
////    if (!pop3_ssl_client.login_to_pop3_server(username, password)) {
////        cout << "Login to POP3_SSL server failed." << endl;
////        return 1;
////    }
////
////    // 获取邮件列表，将邮件列表中每一封邮件的主题和日期输出到info.txt文件中
////    pop3_ssl_client.retrieve_and_print_email_info(pop3_ssl_client);
////
////    // 将第n封邮件的内容保存到email_content.txt中
////    int message_number = 3; // 要查看的邮件的编号，改成用户输入
////    pop3_ssl_client.save_email_content(pop3_ssl_client, message_number);
////
////    //// 删除第n封邮件
////    //int delete_message_number = 1;// 要删除的邮件的编号，改成用户输入
////    //pop3_ssl_client.delete_email(delete_message_number);
////
////    // 关闭连接，使删除操作生效
////    pop3_ssl_client.close_connection();
////
////    return 0;
////}
