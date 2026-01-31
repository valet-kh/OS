#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sqlite3.h> 
#include <chrono>

struct DBRecord {
    long long timestamp;
    double value;
};

class DBManager {
    sqlite3* _db;
    bool _is_open;

public:
    DBManager() : _db(nullptr), _is_open(false) {}
    ~DBManager() { Close(); }

    bool Open(const std::string& path) {
        if (sqlite3_open(path.c_str(), &_db) != SQLITE_OK) {
            std::cerr << "Error opening db: " << sqlite3_errmsg(_db) << std::endl;
            return false;
        }
        _is_open = true;
        InitializeTables();
        return true;
    }

    void Close() {
        if (_is_open && _db) {
            sqlite3_close(_db);
            _db = nullptr;
            _is_open = false;
        }
    }

    // Создание таблиц
    void InitializeTables() {
        const char* sql = 
            "CREATE TABLE IF NOT EXISTS raw_data ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "timestamp INTEGER, "
            "value REAL);"
            
            "CREATE TABLE IF NOT EXISTS hourly_stats ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "timestamp INTEGER, "
            "value REAL);"
            
            "CREATE TABLE IF NOT EXISTS daily_stats ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "timestamp INTEGER, "
            "value REAL);";

        Execute(sql);
    }

    void Insert(const std::string& table, long long ts, double val) {
        std::string sql = "INSERT INTO " + table + " (timestamp, value) VALUES (" + 
                          std::to_string(ts) + ", " + std::to_string(val) + ");";
        Execute(sql);
    }

    void CleanupOldData(const std::string& table, long long threshold_ts) {
        std::string sql = "DELETE FROM " + table + " WHERE timestamp < " + std::to_string(threshold_ts) + ";";
        Execute(sql);
    }

    double GetAverage(const std::string& table, long long since_ts) {
        std::string sql = "SELECT AVG(value) FROM " + table + " WHERE timestamp >= " + std::to_string(since_ts) + ";";
        sqlite3_stmt* stmt;
        double result = 0.0;
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                result = sqlite3_column_double(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<DBRecord> GetAll(const std::string& table, int limit = 100) {
        std::vector<DBRecord> data;
        std::string sql = "SELECT timestamp, value FROM " + table + " ORDER BY timestamp DESC LIMIT " + std::to_string(limit) + ";";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                DBRecord rec;
                rec.timestamp = sqlite3_column_int64(stmt, 0);
                rec.value = sqlite3_column_double(stmt, 1);
                data.push_back(rec);
            }
        }
        sqlite3_finalize(stmt);
        return data;
    }

private:
    void Execute(const std::string& sql) {
        char* errMsg = 0;
        if (sqlite3_exec(_db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
            std::cerr << "SQL Error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }
};