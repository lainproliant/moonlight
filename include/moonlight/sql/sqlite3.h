/*
 * sqlite3.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday January 17, 2025
 */

#ifndef __MOONLIGHT_SQLITE3_H
#define __MOONLIGHT_SQLITE3_H

#include <sqlite3.h>
#include "moonlight/sql.h"

namespace moonlight {
namespace sqlite {

EXCEPTION_SUBTYPE(sql::Error, Error);
EXCEPTION_SUBTYPE(Error, DataError);
EXCEPTION_SUBTYPE(Error, ConnectError);
EXCEPTION_SUBTYPE(Error, StatementError);
EXCEPTION_SUBTYPE(Error, BindError);
EXCEPTION_SUBTYPE(Error, QueryError);
EXCEPTION_SUBTYPE(Error, IterationError);

//-------------------------------------------------------------------
class Column : public sql::Column {
public:
    Column(sqlite3_stmt* statement, int offset)
    : _statement(statement), _offset(offset) { }

    std::string name() const override {
        return std::string(sqlite3_column_name(_statement, _offset));
    }

    int offset() const override {
        return _offset;
    }

    bool is_null() const override {
auto type = sqlite3_column_type(_statement, _offset);
        return type == SQLITE_NULL;
    }

    std::string as_str() const override {
        if (is_null()) THROW(DataError, "Result row is NULL.");

        return std::string(
            reinterpret_cast<const char*>(
                sqlite3_column_text(_statement, _offset)));
    }

    int as_int() const override {
        if (is_null()) THROW(DataError, "Result row is NULL.");
        return sqlite3_column_int(_statement, _offset);
    }

    double as_double() const override {
        if (is_null()) THROW(DataError, "Result row is NULL.");
        return sqlite3_column_double(_statement, _offset);
    }

    float as_float() const override {
        return as_double();
    }

    std::vector<char> as_bytes() const override {
        if (is_null()) THROW(DataError, "Result row is NULL.");
        const char* data = reinterpret_cast<const char*>(
            sqlite3_column_blob(_statement, _offset));
        int size = sqlite3_column_bytes(_statement, _offset);

        auto result = std::vector<char>(size);
        std::copy(data, data + size, result.begin());
        return result;
    }

private:
    sqlite3_stmt* _statement;
    int _offset;
};

//-------------------------------------------------------------------
class Row : public sql::Row {
 public:
     Row(sqlite3_stmt* statement)
     : _statement(statement) {
         int size = sqlite3_data_count(statement);
         for (int x = 0; x < size; x++) {
             add_column(std::make_shared<Column>(_statement, x));
         }
     }

 private:
     sqlite3_stmt* _statement;
};

//-------------------------------------------------------------------
class Client : public sql::Client {
 public:
     Client(struct sqlite3* conn)
     : _conn(conn) { }

     virtual ~Client() {
         close();
     }

     static sql::Client::Pointer open(const std::string& filename) {
         struct sqlite3* conn;

         int result = sqlite3_open_v2(filename.c_str(),
                                      &conn,
                                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                      nullptr);

         if (result != SQLITE_OK) {
             auto message = std::string(sqlite3_errmsg(conn));
             THROW(ConnectError, message);
         }

         return std::make_shared<Client>(conn);
     }

     moonlight::gen::Stream<sql::Row::Pointer> vquery(const std::string& query,
                                                      const std::vector<std::string>& params) override {
         int status;
         sqlite3_stmt* statement = nullptr;

         status = sqlite3_prepare_v2(
             _conn,
             query.c_str(),
             query.size(),
             &statement,
             nullptr
         );

         if (status != SQLITE_OK) {
             auto message = std::string(sqlite3_errstr(status));
             THROW(StatementError, message);
         }

         for (int x = 0; x < params.size(); x++) {
             status = sqlite3_bind_text(statement,
                                        x + 1,
                                        params[x].c_str(),
                                        params[x].size(),
                                        SQLITE_STATIC);

             if (status != SQLITE_OK) {
                 auto message = std::string(sqlite3_errstr(status));
                 THROW(BindError, message);
             }
         }

         status = sqlite3_step(statement);

         switch (status) {
         case SQLITE_OK:
         case SQLITE_DONE:
             return moonlight::gen::nothing<sql::Row::Pointer>();
         case SQLITE_ROW:
             break;
         default:
             THROW(QueryError, std::string(sqlite3_errstr(status)));
         }

         bool finalized = false;
         bool initial = true;

         auto gen_rows = [=]() mutable -> std::optional<sql::Row::Pointer> {
             if (finalized) {
                 return {};
             }

             if (! initial) {
                 status = sqlite3_step(statement);
             } else {
                 initial = false;
             }

             if (status == SQLITE_ROW) {
                 return std::make_shared<Row>(statement);

             } else if (status == SQLITE_DONE) {
                 sqlite3_finalize(statement);
                 return {};

             } else {
                 THROW(IterationError, std::string(sqlite3_errstr(status)));
             }
         };

         return moonlight::gen::stream<sql::Row::Pointer>(gen_rows);
     }

     void close() override {
         sqlite3_close_v2(_conn);
     }

 private:
     struct sqlite3* _conn;
};

}
}


#endif /* !__MOONLIGHT_SQLITE3_H */
