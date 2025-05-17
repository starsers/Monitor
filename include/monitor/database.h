#include <sqlite3.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <mutex>

class SQLiteDatabase {
public:
    // 构造函数
    SQLiteDatabase(const std::string& dbPath) : dbPath(dbPath), db(nullptr) {}
    SQLiteDatabase(const char* dbPath) : dbPath(dbPath), db(nullptr) {}
    SQLiteDatabase(const SQLiteDatabase& db2){
        dbPath = db2.dbPath;
        connect();
    }
    // 析构函数
    ~SQLiteDatabase() {
        if (db) {
            sqlite3_close(db);
        }
    }

    // 连接数据库
    bool connect() {
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        std::cout << "数据库连接成功" << std::endl;
        return true;
    }

    // 同步执行 SQL 查询
    std::vector<std::vector<std::string>> query(const std::string& sql) {
        char* errMsg = nullptr;
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 错误: " << sqlite3_errmsg(db) << std::endl;
            return {};
        }

        std::vector<std::vector<std::string>> result;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::vector<std::string> row;
            int columnCount = sqlite3_column_count(stmt);
            for (int i = 0; i < columnCount; ++i) {
                const unsigned char* value = sqlite3_column_text(stmt, i);
                row.push_back(reinterpret_cast<const char*>(value));
            }
            result.push_back(row);
        }

        sqlite3_finalize(stmt);
        return result;
    }

    // 同步执行 SQL 更新（插入、更新、删除等）
    bool execute(const std::string& sql) {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 执行错误: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

    // 异步执行 SQL 查询
    std::future<std::vector<std::vector<std::string>>> asyncQuery(const std::string& sql) {
        return std::async(std::launch::async, [this, sql]() {
            std::lock_guard<std::mutex> lock(dbMutex);  // 确保线程安全
            return query(sql);
        });
    }

    // 异步执行 SQL 更新
    std::future<bool> asyncExecute(const std::string& sql) {
        return std::async(std::launch::async, [this, sql]() {
            std::lock_guard<std::mutex> lock(dbMutex);  // 确保线程安全
            return execute(sql);
        });
    }

    // 封装常用的 SQL 操作：创建表
    bool createTable(const std::string& tableName, const std::string& columns) {
        std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + " (" + columns + ");";
        return execute(sql);
    }

    // 封装常用的 SQL 操作：插入数据
    bool insertData(const std::string& tableName, const std::string& columns, const std::string& values) {
        std::string sql = "INSERT INTO " + tableName + " (" + columns + ") VALUES (" + values + ");";
        return execute(sql);
    }

    // 封装常用的 SQL 操作：查询所有数据
    std::vector<std::vector<std::string>> selectAllFromTable(const std::string& tableName) {
        std::string sql = "SELECT * FROM " + tableName + ";";
        return query(sql);
    }

    // 封装常用的 SQL 操作：删除数据
    bool deleteDataFromTable(const std::string& tableName, const std::string& condition) {
        std::string sql = "DELETE FROM " + tableName + " WHERE " + condition + ";";
        return execute(sql);
    }

private:
    std::string dbPath;
    sqlite3* db;
    std::mutex dbMutex;
};