#include <iostream>
#include <string>
#include <libpq-fe.h>
#include "DataBase.cpp"

// 点击发送后存储已发送的邮件
void sendEmailAndSaveToDatabase(PostgreSQLDatabase& db, const std::string& sender, const std::string& recipient, const std::string& subject, const std::string& content) {
    // 发送邮件的相关代码

    // 调用数据库对象的函数保存已发送的邮件
    db.saveSentEmail(sender, recipient, subject, content);
}

// 点击发送后判断收件人是否在联系人里，如果不在就添加
void checkRecipientAndAddToContacts(PostgreSQLDatabase& db, const std::string& recipient) {
    // 判断收件人是否在联系人中的相关代码

    // 如果不在联系人中，调用数据库对象的函数添加联系人
    if (!db.contactExists(recipient)) {
        std::string contactName; // 这里可以根据实际情况设置联系人姓名
        db.saveContact(contactName, recipient);
    }
}

// 添加自定的联系人
void addCustomContact(PostgreSQLDatabase& db, const std::string& contactName, const std::string& contactEmail) {
    // 添加自定义联系人的相关代码

    // 调用数据库对象的函数保存自定义联系人
    db.saveContact(contactName, contactEmail);
}

// 收到的邮件查自动保存到数据库中
void saveReceivedEmailToDatabase(PostgreSQLDatabase& db, const std::string& sender, const std::string& recipient, const std::string& subject, const std::string& content) {
    // 处理收到的邮件的相关代码

    // 调用数据库对象的函数保存收到的邮件
    db.saveEmail(sender, recipient, subject, content);
}

// 删除已发送的邮件
void deleteSentEmail(PostgreSQLDatabase& db, int id) {
    // 根据邮件ID删除已发送的邮件的相关代码

    // 调用数据库对象的函数删除已发送的邮件
    db.deleteSentEmailById(id);
}

// 删除收到的邮件
void deleteReceivedEmail(PostgreSQLDatabase& db, int id) {
    // 根据邮件ID删除收到的邮件的相关代码

    // 调用数据库对象的函数删除收到的邮件
    db.deleteEmailById(id);
}

// 删除联系人
void deleteContact(PostgreSQLDatabase& db, int id) {
    // 根据联系人ID删除联系人的相关代码

    // 调用数据库对象的函数删除联系人
    db.deleteContactById(id);
}

//int main() {
//    std::string dbname = "email";
//    std::string user = "lwx";
//    std::string password = "123456";
//    std::string host = "101.37.32.213"; //数据库服务器放在本地
//    std::string port = "5432";
//
//    PostgreSQLDatabase db(dbname, user, password, host, port);
//
//    if (db.connect()) {
//        std::cout << "connected to the database." << std::endl;
//
//        // 创建表
//        if (db.createContactTable()) {
//            std::cout << "tables created successfully." << std::endl;
//        }
//        else {
//            std::cerr << "failed to create tables." << std::endl;
//            return 1;
//        }
//    }
//
//    return 0;
//}