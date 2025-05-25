#include <sqlite3.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <memory>

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
        std::lock_guard<std::mutex> lock(dbMutex);
        return queryInternal(sql);
    }

    // 同步执行 SQL 更新（插入、更新、删除等）
    bool execute(const std::string& sql) {
        std::lock_guard<std::mutex> lock(dbMutex);
        return executeInternal(sql);
    }

    // 异步执行 SQL 查询 - 每个异步操作使用独立的数据库连接
    std::future<std::vector<std::vector<std::string>>> asyncQuery(const std::string& sql) {
        return std::async(std::launch::async, [this, sql]() {
            sqlite3* asyncDb = nullptr;
            int rc = sqlite3_open(dbPath.c_str(), &asyncDb);
            if (rc != SQLITE_OK) {
                std::cerr << "异步查询无法打开数据库: " << sqlite3_errmsg(asyncDb) << std::endl;
                if (asyncDb) sqlite3_close(asyncDb);
                return std::vector<std::vector<std::string>>{};
            }
            
            auto result = queryInternalWithDb(asyncDb, sql);
            sqlite3_close(asyncDb);
            return result;
        });
    }

    // 异步执行 SQL 更新 - 每个异步操作使用独立的数据库连接
    std::future<bool> asyncExecute(const std::string& sql) {
        return std::async(std::launch::async, [this, sql]() {
            sqlite3* asyncDb = nullptr;
            int rc = sqlite3_open(dbPath.c_str(), &asyncDb);
            if (rc != SQLITE_OK) {
                std::cerr << "异步执行无法打开数据库: " << sqlite3_errmsg(asyncDb) << std::endl;
                if (asyncDb) sqlite3_close(asyncDb);
                return false;
            }
            
            auto result = executeInternalWithDb(asyncDb, sql);
            sqlite3_close(asyncDb);
            return result;
        });
    }

    // 封装常用的 SQL 操作：创建表
    bool createTable(const std::string& tableName, const std::string& columns) {
        std::string sql = "CREATE TABLE IF NOT EXISTS " + sanitizeIdentifier(tableName) + " (" + columns + ");";
        return execute(sql);
    }

    // 封装常用的 SQL 操作：插入数据（参数化查询）
    bool insertData(const std::string& tableName, const std::vector<std::string>& columns, const std::vector<std::string>& values) {
        if (columns.size() != values.size()) {
            std::cerr << "列数和值数不匹配" << std::endl;
            return false;
        }

        std::string columnList;
        std::string placeholders;
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) {
                columnList += ", ";
                placeholders += ", ";
            }
            columnList += sanitizeIdentifier(columns[i]);
            placeholders += "?";
        }

        std::string sql = "INSERT INTO " + sanitizeIdentifier(tableName) + " (" + columnList + ") VALUES (" + placeholders + ");";
        return executeWithParams(sql, values);
    }

    // 保持原接口兼容性的插入方法
    bool insertData(const std::string& tableName, const std::string& columns, const std::string& values) {
        std::string sql = "INSERT INTO " + sanitizeIdentifier(tableName) + " (" + columns + ") VALUES (" + values + ");";
        return execute(sql);
    }

    // 封装常用的 SQL 操作：查询所有数据
    std::vector<std::vector<std::string>> selectAllFromTable(const std::string& tableName) {
        std::string sql = "SELECT * FROM " + sanitizeIdentifier(tableName) + ";";
        return query(sql);
    }

    // 封装常用的 SQL 操作：删除数据
    bool deleteDataFromTable(const std::string& tableName, const std::string& condition) {
        std::string sql = "DELETE FROM " + sanitizeIdentifier(tableName) + " WHERE " + condition + ";";
        return execute(sql);
    }

    // 新增：参数化查询方法
    std::vector<std::vector<std::string>> queryWithParams(const std::string& sql, const std::vector<std::string>& params) {
        std::lock_guard<std::mutex> lock(dbMutex);
        return queryWithParamsInternal(db, sql, params);
    }

    // 新增：参数化执行方法
    bool executeWithParams(const std::string& sql, const std::vector<std::string>& params) {
        std::lock_guard<std::mutex> lock(dbMutex);
        return executeWithParamsInternal(db, sql, params);
    }

private:
    std::string dbPath;
    sqlite3* db;
    std::mutex dbMutex;

    // 内部查询实现
    std::vector<std::vector<std::string>> queryInternal(const std::string& sql) {
        return queryInternalWithDb(db, sql);
    }

    // 内部执行实现
    bool executeInternal(const std::string& sql) {
        return executeInternalWithDb(db, sql);
    }

    // 使用指定数据库连接的查询实现
    std::vector<std::vector<std::string>> queryInternalWithDb(sqlite3* database, const std::string& sql) {
        sqlite3_stmt* stmt = nullptr;
        
        int rc = sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 错误: " << sqlite3_errmsg(database) << std::endl;
            return {};
        }

        std::vector<std::vector<std::string>> result;
        try {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::vector<std::string> row;
                int columnCount = sqlite3_column_count(stmt);
                for (int i = 0; i < columnCount; ++i) {
                    const unsigned char* value = sqlite3_column_text(stmt, i);
                    row.push_back(value ? reinterpret_cast<const char*>(value) : "");
                }
                result.push_back(row);
            }
        } catch (...) {
            sqlite3_finalize(stmt);
            throw;
        }

        sqlite3_finalize(stmt);
        return result;
    }

    // 使用指定数据库连接的执行实现
    bool executeInternalWithDb(sqlite3* database, const std::string& sql) {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 执行错误: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

    // 参数化查询内部实现
    std::vector<std::vector<std::string>> queryWithParamsInternal(sqlite3* database, const std::string& sql, const std::vector<std::string>& params) {
        sqlite3_stmt* stmt = nullptr;
        
        int rc = sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 错误: " << sqlite3_errmsg(database) << std::endl;
            return {};
        }

        // 绑定参数
        for (size_t i = 0; i < params.size(); ++i) {
            rc = sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_STATIC);
            if (rc != SQLITE_OK) {
                std::cerr << "参数绑定错误: " << sqlite3_errmsg(database) << std::endl;
                sqlite3_finalize(stmt);
                return {};
            }
        }

        std::vector<std::vector<std::string>> result;
        try {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::vector<std::string> row;
                int columnCount = sqlite3_column_count(stmt);
                for (int i = 0; i < columnCount; ++i) {
                    const unsigned char* value = sqlite3_column_text(stmt, i);
                    row.push_back(value ? reinterpret_cast<const char*>(value) : "");
                }
                result.push_back(row);
            }
        } catch (...) {
            sqlite3_finalize(stmt);
            throw;
        }

        sqlite3_finalize(stmt);
        return result;
    }

    // 参数化执行内部实现
    bool executeWithParamsInternal(sqlite3* database, const std::string& sql, const std::vector<std::string>& params) {
        sqlite3_stmt* stmt = nullptr;
        
        int rc = sqlite3_prepare_v2(database, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL 错误: " << sqlite3_errmsg(database) << std::endl;
            return false;
        }

        // 绑定参数
        for (size_t i = 0; i < params.size(); ++i) {
            rc = sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_STATIC);
            if (rc != SQLITE_OK) {
                std::cerr << "参数绑定错误: " << sqlite3_errmsg(database) << std::endl;
                sqlite3_finalize(stmt);
                return false;
            }
        }

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            std::cerr << "SQL 执行错误: " << sqlite3_errmsg(database) << std::endl;
            return false;
        }
        return true;
    }

    // 标识符清理（防止SQL注入）
    std::string sanitizeIdentifier(const std::string& identifier) {
        std::string result;
        for (char c : identifier) {
            if (std::isalnum(c) || c == '_') {
                result += c;
            }
        }
        return result;
    }
};