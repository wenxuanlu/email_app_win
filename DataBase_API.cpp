#include <iostream>
#include <string>
#include <libpq-fe.h>
#include "DataBase.cpp"

// ������ͺ�洢�ѷ��͵��ʼ�
void sendEmailAndSaveToDatabase(PostgreSQLDatabase& db, const std::string& sender, const std::string& recipient, const std::string& subject, const std::string& content) {
    // �����ʼ�����ش���

    // �������ݿ����ĺ��������ѷ��͵��ʼ�
    db.saveSentEmail(sender, recipient, subject, content);
}

// ������ͺ��ж��ռ����Ƿ�����ϵ���������ھ����
void checkRecipientAndAddToContacts(PostgreSQLDatabase& db, const std::string& recipient) {
    // �ж��ռ����Ƿ�����ϵ���е���ش���

    // ���������ϵ���У��������ݿ����ĺ��������ϵ��
    if (!db.contactExists(recipient)) {
        std::string contactName; // ������Ը���ʵ�����������ϵ������
        db.saveContact(contactName, recipient);
    }
}

// ����Զ�����ϵ��
void addCustomContact(PostgreSQLDatabase& db, const std::string& contactName, const std::string& contactEmail) {
    // ����Զ�����ϵ�˵���ش���

    // �������ݿ����ĺ��������Զ�����ϵ��
    db.saveContact(contactName, contactEmail);
}

// �յ����ʼ����Զ����浽���ݿ���
void saveReceivedEmailToDatabase(PostgreSQLDatabase& db, const std::string& sender, const std::string& recipient, const std::string& subject, const std::string& content) {
    // �����յ����ʼ�����ش���

    // �������ݿ����ĺ��������յ����ʼ�
    db.saveEmail(sender, recipient, subject, content);
}

// ɾ���ѷ��͵��ʼ�
void deleteSentEmail(PostgreSQLDatabase& db, int id) {
    // �����ʼ�IDɾ���ѷ��͵��ʼ�����ش���

    // �������ݿ����ĺ���ɾ���ѷ��͵��ʼ�
    db.deleteSentEmailById(id);
}

// ɾ���յ����ʼ�
void deleteReceivedEmail(PostgreSQLDatabase& db, int id) {
    // �����ʼ�IDɾ���յ����ʼ�����ش���

    // �������ݿ����ĺ���ɾ���յ����ʼ�
    db.deleteEmailById(id);
}

// ɾ����ϵ��
void deleteContact(PostgreSQLDatabase& db, int id) {
    // ������ϵ��IDɾ����ϵ�˵���ش���

    // �������ݿ����ĺ���ɾ����ϵ��
    db.deleteContactById(id);
}

//int main() {
//    std::string dbname = "email";
//    std::string user = "lwx";
//    std::string password = "123456";
//    std::string host = "101.37.32.213"; //���ݿ���������ڱ���
//    std::string port = "5432";
//
//    PostgreSQLDatabase db(dbname, user, password, host, port);
//
//    if (db.connect()) {
//        std::cout << "connected to the database." << std::endl;
//
//        // ������
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