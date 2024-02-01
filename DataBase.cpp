#include <iostream>
#include <string>
#include <vector>
#include <libpq-fe.h>

// 定义邮件数据的结构体
struct Emails {
    int id;
    std::string sender;
    std::string recipient;
    std::string subject;
    std::string content;
    std::string timestamp;
};

// 定义联系人数据的结构体
struct Contact {
    int id;
    std::string name;
    std::string email;
};

class PostgreSQLDatabase {
public:
    PostgreSQLDatabase(const std::string& dbname, const std::string& user, const std::string& password, const std::string& host, const std::string& port)
        : dbname(dbname), user(user), password(password), host(host), port(port), connection(nullptr) {}

    ~PostgreSQLDatabase() {
        disconnect();
    }

    bool connect() {
        std::string connectionInfo = "dbname=" + dbname + " user=" + user + " password=" + password + " host=" + host + " port=" + port;
        connection = PQconnectdb(connectionInfo.c_str());

        if (PQstatus(connection) == CONNECTION_OK) {
            std::cout << "Connected to database successfully." << std::endl;
            return true;
        }
        else {
            std::cerr << "Connection to database failed: " << PQerrorMessage(connection) << std::endl;
            disconnect();
            return false;
        }
    }

    void disconnect() {
        if (connection) {
            PQfinish(connection);
            connection = nullptr;
            std::cout << "Disconnected from database." << std::endl;
        }
    }

    bool createTables() {
        // ���� emails ��
        std::string createEmailTableQuery = "CREATE TABLE IF NOT EXISTS emails ("
            "id SERIAL PRIMARY KEY,"
            "sender VARCHAR(100) NOT NULL,"
            "recipient VARCHAR(100) NOT NULL,"
            "subject VARCHAR(255) NOT NULL,"
            "content TEXT,"
            "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";

        if (!executeQuery(createEmailTableQuery)) {
            return false;
        }

        // ���� sent_emails ��
        std::string createSentEmailTableQuery = "CREATE TABLE IF NOT EXISTS sent_emails ("
            "id SERIAL PRIMARY KEY,"
            "sender VARCHAR(100) NOT NULL,"
            "recipient VARCHAR(100) NOT NULL,"
            "subject VARCHAR(255) NOT NULL,"
            "content TEXT,"
            "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";

        return executeQuery(createSentEmailTableQuery);
    }

    bool saveEmail(const std::string& sender, const std::string& recipient, const std::string& subject, const std::string& content) {
        std::string insertEmailQuery = "INSERT INTO emails (sender, recipient, subject, content) VALUES ("
            "'" + sender + "',"
            "'" + recipient + "',"
            "'" + subject + "',"
            "'" + content + "')";

        return executeQuery(insertEmailQuery);
    }

    bool saveSentEmail(const std::string& sender, const std::string& recipient, const std::string& subject, const std::string& content) {
        std::string insertSentEmailQuery = "INSERT INTO sent_emails (sender, recipient, subject, content) VALUES ("
            "'" + sender + "',"
            "'" + recipient + "',"
            "'" + subject + "',"
            "'" + content + "')";

        return executeQuery(insertSentEmailQuery);
    }

    //用来调试
    // void getAllEmails() {
    //     std::string selectAllEmailsQuery = "SELECT * FROM emails";

    //     PGresult* result = PQexec(connection, selectAllEmailsQuery.c_str());

    //     if (PQresultStatus(result) == PGRES_TUPLES_OK) {
    //         int numRows = PQntuples(result);

    //         for (int i = 0; i < numRows; i++) {
    //             int id = std::stoi(PQgetvalue(result, i, 0));
    //             std::string sender = PQgetvalue(result, i, 1);
    //             std::string recipient = PQgetvalue(result, i, 2);
    //             std::string subject = PQgetvalue(result, i, 3);
    //             std::string content = PQgetvalue(result, i, 4);
    //             std::string timestamp = PQgetvalue(result, i, 5);

    //             std::cout << "ID: " << id << ", Sender: " << sender << ", Recipient: " << recipient
    //                 << ", Subject: " << subject << ", Content: " << content << ", Timestamp: " << timestamp << std::endl;
    //         }
    //     }

    //     PQclear(result);
    // }
    
    //用来提供接口
    std::vector<Emails> getAllEmails() {
        std::vector<Emails> emails;
        std::string selectAllEmailsQuery = "SELECT * FROM emails";

        PGresult* result = PQexec(connection, selectAllEmailsQuery.c_str());

        if (PQresultStatus(result) == PGRES_TUPLES_OK) {
            int numRows = PQntuples(result);

            for (int i = 0; i < numRows; i++) {
                Emails email;
                email.id = std::stoi(PQgetvalue(result, i, 0));
                email.sender = PQgetvalue(result, i, 1);
                email.recipient = PQgetvalue(result, i, 2);
                email.subject = PQgetvalue(result, i, 3);
                email.content = PQgetvalue(result, i, 4);
                email.timestamp = PQgetvalue(result, i, 5);

                emails.push_back(email);
            }
        }

        PQclear(result);
        return emails;
    }

    //用来调试
    // void getAllSentEmails() {
    //     std::string selectAllSentEmailsQuery = "SELECT * FROM sent_emails";

    //     PGresult* result = PQexec(connection, selectAllSentEmailsQuery.c_str());

    //     if (PQresultStatus(result) == PGRES_TUPLES_OK) {
    //         int numRows = PQntuples(result);

    //         for (int i = 0; i < numRows; i++) {
    //             int id = std::stoi(PQgetvalue(result, i, 0));
    //             std::string sender = PQgetvalue(result, i, 1);
    //             std::string recipient = PQgetvalue(result, i, 2);
    //             std::string subject = PQgetvalue(result, i, 3);
    //             std::string content = PQgetvalue(result, i, 4);
    //             std::string timestamp = PQgetvalue(result, i, 5);

    //             std::cout << "ID: " << id << ", Sender: " << sender << ", Recipient: " << recipient
    //                 << ", Subject: " << subject << ", Content: " << content << ", Timestamp: " << timestamp << std::endl;
    //         }
    //     }

    //     PQclear(result);
    // }

    //用来提供接口
    std::vector<Emails> getAllSentEmails() {
        std::vector<Emails> sentEmails;
        std::string selectAllSentEmailsQuery = "SELECT * FROM sent_emails";

        PGresult* result = PQexec(connection, selectAllSentEmailsQuery.c_str());

        if (PQresultStatus(result) == PGRES_TUPLES_OK) {
            int numRows = PQntuples(result);

            for (int i = 0; i < numRows; i++) {
                Emails email;
                email.id = std::stoi(PQgetvalue(result, i, 0));
                email.sender = PQgetvalue(result, i, 1);
                email.recipient = PQgetvalue(result, i, 2);
                email.subject = PQgetvalue(result, i, 3);
                email.content = PQgetvalue(result, i, 4);
                email.timestamp = PQgetvalue(result, i, 5);

                sentEmails.push_back(email);
            }
        }

        PQclear(result);
        return sentEmails;
    }


    bool deleteEmailById(int id) {
        std::string deleteEmailQuery = "DELETE FROM emails WHERE id = " + std::to_string(id);
        return executeQuery(deleteEmailQuery);
    }

    bool deleteSentEmailById(int id) {
        std::string deleteSentEmailQuery = "DELETE FROM sent_emails WHERE id = " + std::to_string(id);
        return executeQuery(deleteSentEmailQuery);
    }

    bool createContactTable() {
        std::string createContactTableQuery = "CREATE TABLE IF NOT EXISTS contacts ("
            "id SERIAL PRIMARY KEY,"
            "name VARCHAR(100) NOT NULL,"
            "email VARCHAR(100) NOT NULL)";

        return executeQuery(createContactTableQuery);
    }

    bool saveContact(const std::string& name, const std::string& email) {
        std::string insertContactQuery = "INSERT INTO contacts (name, email) VALUES ("
            "'" + name + "',"
            "'" + email + "')";

        return executeQuery(insertContactQuery);
    }
    //用来调试
    // void getAllContacts() {
    //     std::string selectAllContactsQuery = "SELECT * FROM contacts";

    //     PGresult* result = PQexec(connection, selectAllContactsQuery.c_str());

    //     if (PQresultStatus(result) == PGRES_TUPLES_OK) {
    //         int numRows = PQntuples(result);

    //         for (int i = 0; i < numRows; i++) {
    //             int id = std::stoi(PQgetvalue(result, i, 0));
    //             std::string name = PQgetvalue(result, i, 1);
    //             std::string email = PQgetvalue(result, i, 2);

    //             std::cout << "ID: " << id << ", Name: " << name << ", Email: " << email << std::endl;
    //         }
    //     }

    //     PQclear(result);
    // }

    //用来提供接口
    std::vector<Contact> getAllContacts() {
        std::vector<Contact> contacts;
        std::string selectAllContactsQuery = "SELECT * FROM contacts";

        PGresult* result = PQexec(connection, selectAllContactsQuery.c_str());

        if (PQresultStatus(result) == PGRES_TUPLES_OK) {
            int numRows = PQntuples(result);

            for (int i = 0; i < numRows; i++) {
                Contact contact;
                contact.id = std::stoi(PQgetvalue(result, i, 0));
                contact.name = PQgetvalue(result, i, 1);
                contact.email = PQgetvalue(result, i, 2);

                contacts.push_back(contact);
            }
        }

        PQclear(result);
        return contacts;
    }


    bool deleteContactById(int id) {
        std::string deleteContactQuery = "DELETE FROM contacts WHERE id = " + std::to_string(id);
        return executeQuery(deleteContactQuery);
    }


    bool contactExists(const std::string& email) {
        std::string query = "SELECT COUNT(*) FROM contacts WHERE email = '" + email + "'";

        PGresult* result = PQexec(connection, query.c_str());

        if (PQresultStatus(result) == PGRES_TUPLES_OK) {
            int rowCount = PQntuples(result);

            if (rowCount > 0) {
                int count = std::stoi(PQgetvalue(result, 0, 0));
                PQclear(result);
                return (count > 0);
            }
        }

        PQclear(result);
        return false;
    }



private:
    std::string dbname;
    std::string user;
    std::string password;
    std::string host;
    std::string port;
    PGconn* connection;

    bool executeQuery(const std::string& query) {
        if (!connection) {
            std::cerr << "Not connected to the database." << std::endl;
            return false;
        }

        PGresult* result = PQexec(connection, query.c_str());

        if (PQresultStatus(result) != PGRES_COMMAND_OK && PQresultStatus(result) != PGRES_TUPLES_OK) {
            std::cerr << "Query execution failed: " << PQerrorMessage(connection) << std::endl;
            PQclear(result);
            return false;
        }

        PQclear(result);
        return true;
    }
};
