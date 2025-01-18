/*
 *
 * sql.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday January 17, 2025
 */

#ifndef __MOONLIGHT_SQL_H
#define __MOONLIGHT_SQL_H

#include <format>
#include "moonlight/linked_map.h"
#include "moonlight/generator.h"

namespace moonlight {
namespace sql {

//-------------------------------------------------------------------
EXCEPTION_TYPE(Error);

//-------------------------------------------------------------------
class Column {
public:
    virtual ~Column() { }

    typedef std::shared_ptr<Column> Pointer;

    template<class T>
    T get() const {
        static_assert(always_false<T>(), "Column value can't be converted to the given type.");
    }

    template<>
    std::string get<std::string>() const {
        return as_str();
    }

    template<>
    int get<int>() const {
        return as_int();
    }

    template<>
    double get<double>() const {
        return as_double();
    }

    template<>
    float get<float>() const {
        return as_float();
    }

    template<>
    std::vector<char> get<std::vector<char>>() const {
        return as_bytes();
    }

    virtual std::string name() const = 0;
    virtual int offset() const = 0;
    virtual bool is_null() const = 0;

    virtual std::string as_str() const = 0;
    virtual int as_int() const = 0;
    virtual double as_double() const = 0;
    virtual float as_float() const = 0;
    virtual std::vector<char> as_bytes() const = 0;
};

//-------------------------------------------------------------------
class Row {
public:
    typedef std::shared_ptr<Row> Pointer;

    virtual ~Row() { }

    int length() const {
        return _columns.size();
    }

    Column& at(int offset) const {
        if (offset >= length()) {
            THROW(Error, std::format("Column offset out of bounds: {}", offset));
        }
        return *_columns.at_offset(offset);
    }

    Column& at(const std::string& key) const {
        auto iter = _columns.find(key);
        if (iter == _columns.end()) {
            THROW(Error, std::format("Column not found: '{}'", key));
        }
        return *iter->second;
    }

    Column& at(const char* name) const {
        auto iter = _columns.find(std::string(name));
        if (iter == _columns.end()) {
            THROW(Error, std::format("Column not found: '{}'", name));
        }
        return *iter->second;
    }

protected:
    void add_column(Column::Pointer column) {
        _columns.insert({column->name(), column});
    }

private:
    moonlight::linked_map<std::string, Column::Pointer> _columns;
};

//-------------------------------------------------------------------
class Client {
public:
    typedef std::shared_ptr<Client> Pointer;

    virtual ~Client() { }

    template<class... TD>
    void exec(const std::string& q, const TD&... values) {
        return query(q, values...).drain();
    }

    template<class... TD>
    moonlight::gen::Stream<Row::Pointer> query(const std::string& q, const TD&... values) {
        std::vector<std::string> v;
        _prep(v, values...);
        return vquery(q, v);
    }

    virtual moonlight::gen::Stream<Row::Pointer> vquery(
        const std::string& query,
        const std::vector<std::string>& params = {}) = 0;
    virtual void close() = 0;

private:
    template<class T, class... TD>
    void _prep(std::vector<std::string>& v, const T& value, const TD&... values) {
        std::ostringstream sb;
        sb << value;
        v.push_back(sb.str());
        _prep(v, values...);
    }

    void _prep(std::vector<std::string>& v) { }
};

}
}

#endif /* !__MOONLIGHT_SQL_H */
